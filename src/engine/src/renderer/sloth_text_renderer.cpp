#include "sloth_text_renderer.h"

#include "font/sloth_font.h"
#include "renderer/sloth_glyph_cache.h"
#include "renderer/sloth_shader.h"

#include <glad/gl.h>

namespace sloth {
    static const char * TextVertexShaderSource = R"(
        #version 450 core
        layout(location = 0) in vec2 aUnit;
        layout(location = 1) in vec2 aQuadMin;
        layout(location = 2) in vec2 aQuadMax;
        layout(location = 3) in vec2 aCurveMin;
        layout(location = 4) in vec2 aCurveMax;
        layout(location = 5) in uint aCurveOffset;
        layout(location = 6) in uint aCurveCount;
        layout(location = 7) in vec4 aColor;

        uniform mat4 uViewProjection;

        out vec2 vCurvePos;
        flat out uint vCurveOffset;
        flat out uint vCurveCount;
        out vec4 vColor;

        void main()
        {
            vec2 pixelPos = mix(aQuadMin, aQuadMax, aUnit);
            vCurvePos = mix(aCurveMin, aCurveMax, aUnit);
            vCurveOffset = aCurveOffset;
            vCurveCount = aCurveCount;
            vColor = aColor;
            gl_Position = uViewProjection * vec4(pixelPos, 0.0, 1.0);
        }
    )";

    // Brute-force per-pixel curve evaluation (the core Slug idea) without
    // the horizontal/vertical band accelerator: every curve in the glyph is
    // tested against every subsample. A nonzero-winding-number ray cast
    // (horizontal ray in +x from the sample point) determines inside/
    // outside per subsample; a small stratified grid built from the screen-
    // space derivatives of the curve-space position approximates
    // antialiasing (Slug proper computes this analytically - this is a
    // simpler, more expensive stand-in for a first pass).
    static const char * TextFragmentShaderSource = R"(
        #version 450 core

        in vec2 vCurvePos;
        flat in uint vCurveOffset;
        flat in uint vCurveCount;
        in vec4 vColor;

        uniform samplerBuffer uCurves;

        out vec4 FragColor;

        vec2 FetchPoint(uint index)
        {
            return texelFetch(uCurves, int(index)).xy;
        }

        // Nonzero winding number contribution of one quadratic Bezier curve
        // against a horizontal ray cast from `p` towards +x.
        float CurveWinding(vec2 p, vec2 p0, vec2 p1, vec2 p2)
        {
            float winding = 0.0;

            float a = p0.y - 2.0 * p1.y + p2.y;
            float b = 2.0 * (p1.y - p0.y);
            float c = p0.y - p.y;

            if (abs(a) < 1e-6)
            {
                if (abs(b) > 1e-6)
                {
                    float t = -c / b;
                    if (t >= 0.0 && t <= 1.0)
                    {
                        float x = mix(mix(p0.x, p1.x, t), mix(p1.x, p2.x, t), t);
                        if (x > p.x)
                        {
                            winding += b > 0.0 ? 1.0 : -1.0;
                        }
                    }
                }
            }
            else
            {
                float disc = b * b - 4.0 * a * c;
                if (disc >= 0.0)
                {
                    float sq = sqrt(disc);
                    float roots[2] = float[2]((-b - sq) / (2.0 * a), (-b + sq) / (2.0 * a));

                    for (int i = 0; i < 2; ++i)
                    {
                        float t = roots[i];
                        if (t >= 0.0 && t <= 1.0)
                        {
                            float x = mix(mix(p0.x, p1.x, t), mix(p1.x, p2.x, t), t);
                            if (x > p.x)
                            {
                                float dy = 2.0 * a * t + b;
                                winding += dy > 0.0 ? 1.0 : -1.0;
                            }
                        }
                    }
                }
            }

            return winding;
        }

        bool IsInside(vec2 p)
        {
            float winding = 0.0;
            for (uint i = 0u; i < vCurveCount; ++i)
            {
                uint base = (vCurveOffset + i) * 3u;
                vec2 p0 = FetchPoint(base + 0u);
                vec2 p1 = FetchPoint(base + 1u);
                vec2 p2 = FetchPoint(base + 2u);
                winding += CurveWinding(p, p0, p1, p2);
            }
            return abs(winding) > 0.5;
        }

        void main()
        {
            vec2 dx = dFdx(vCurvePos);
            vec2 dy = dFdy(vCurvePos);

            const int kSamples = 4;
            float coverage = 0.0;
            for (int sy = 0; sy < kSamples; ++sy)
            {
                for (int sx = 0; sx < kSamples; ++sx)
                {
                    vec2 offset = ((vec2(sx, sy) + 0.5) / float(kSamples)) - 0.5;
                    vec2 samplePos = vCurvePos + dx * offset.x + dy * offset.y;
                    if (IsInside(samplePos))
                    {
                        coverage += 1.0;
                    }
                }
            }
            coverage /= float(kSamples * kSamples);

            if (coverage <= 0.0)
            {
                discard;
            }

            FragColor = vec4(vColor.rgb, vColor.a * coverage);
        }
    )";

    TextRenderer::TextRenderer() {
        shader = std::make_unique<Shader>( TextVertexShaderSource, TextFragmentShaderSource );

        const f32 unitQuad[] = {
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
        };

        glGenVertexArrays( 1, &quadVertexArray );
        glBindVertexArray( quadVertexArray );

        glGenBuffers( 1, &quadVertexBuffer );
        glBindBuffer( GL_ARRAY_BUFFER, quadVertexBuffer );
        glBufferData( GL_ARRAY_BUFFER, sizeof( unitQuad ), unitQuad, GL_STATIC_DRAW );
        glEnableVertexAttribArray( 0 );
        glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, sizeof( f32 ) * 2, reinterpret_cast<void *>( 0 ) );

        glGenBuffers( 1, &instanceBuffer );
        glBindBuffer( GL_ARRAY_BUFFER, instanceBuffer );

        auto instanceAttrib = []( u32 location, i32 componentCount, GLenum type, usize offset ) {
            glEnableVertexAttribArray( location );
            if ( type == GL_UNSIGNED_INT ) {
                glVertexAttribIPointer( location, componentCount, type, sizeof( GlyphInstance ), reinterpret_cast<void *>( offset ) );
            } else {
                glVertexAttribPointer( location, componentCount, type, GL_FALSE, sizeof( GlyphInstance ), reinterpret_cast<void *>( offset ) );
            }
            glVertexAttribDivisor( location, 1 );
        };

        instanceAttrib( 1, 2, GL_FLOAT, offsetof( GlyphInstance, QuadMin ) );
        instanceAttrib( 2, 2, GL_FLOAT, offsetof( GlyphInstance, QuadMax ) );
        instanceAttrib( 3, 2, GL_FLOAT, offsetof( GlyphInstance, CurveMin ) );
        instanceAttrib( 4, 2, GL_FLOAT, offsetof( GlyphInstance, CurveMax ) );
        instanceAttrib( 5, 1, GL_UNSIGNED_INT, offsetof( GlyphInstance, CurveOffset ) );
        instanceAttrib( 6, 1, GL_UNSIGNED_INT, offsetof( GlyphInstance, CurveCount ) );
        instanceAttrib( 7, 4, GL_FLOAT, offsetof( GlyphInstance, Color ) );

        glBindVertexArray( 0 );
    }

    TextRenderer::~TextRenderer() {
        glDeleteBuffers( 1, &instanceBuffer );
        glDeleteBuffers( 1, &quadVertexBuffer );
        glDeleteVertexArrays( 1, &quadVertexArray );
    }

    void TextRenderer::DrawText( const Font & font, GlyphCache & cache, StringView text, glm::vec2 baselinePos, f32 pixelHeight,
        const glm::vec4 & color, const glm::mat4 & viewProjection ) {
        instances.clear();

        f32 scale = font.GetScaleForPixelHeight( pixelHeight );
        glm::vec2 cursor = baselinePos;

        for ( usize i = 0; i < text.Length(); ++i ) {
            u32 codepoint = static_cast<u8>( text[i] );
            i32 glyphIndex = font.GetGlyphIndex( codepoint );
            const CachedGlyph & glyph = cache.GetOrAddGlyph( font, glyphIndex );

            if ( glyph.curveCount > 0 ) {
                GlyphInstance instance;
                // Glyph-space Y grows up, screen-space Y grows down - flip
                // when placing the quad so glyphs aren't rendered upside down.
                instance.QuadMin = cursor + glm::vec2( glyph.xMin, -glyph.yMax ) * scale;
                instance.QuadMax = cursor + glm::vec2( glyph.xMax, -glyph.yMin ) * scale;
                instance.CurveMin = glm::vec2( glyph.xMin, glyph.yMax );
                instance.CurveMax = glm::vec2( glyph.xMax, glyph.yMin );
                instance.CurveOffset = glyph.curveOffset;
                instance.CurveCount = glyph.curveCount;
                instance.Color = color;
                instances.push_back( instance );
            }

            cursor.x += static_cast<f32>( glyph.advanceWidth ) * scale;
        }

        if ( instances.empty() ) {
            return;
        }

        glBindBuffer( GL_ARRAY_BUFFER, instanceBuffer );
        usize requiredBytes = instances.size() * sizeof( GlyphInstance );
        if ( requiredBytes > instanceBufferCapacity ) {
            instanceBufferCapacity = requiredBytes;
            glBufferData( GL_ARRAY_BUFFER, static_cast<GLsizeiptr>( instanceBufferCapacity ), instances.data(), GL_DYNAMIC_DRAW );
        } else {
            glBufferSubData( GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>( requiredBytes ), instances.data() );
        }

        bool blendWasEnabled = glIsEnabled( GL_BLEND );
        bool depthTestWasEnabled = glIsEnabled( GL_DEPTH_TEST );

        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable( GL_DEPTH_TEST );

        shader->Bind();
        shader->SetMat4( "uViewProjection", viewProjection );
        shader->SetInt( "uCurves", 0 );
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_BUFFER, cache.GetCurveTextureId() );

        glBindVertexArray( quadVertexArray );
        glDrawArraysInstanced( GL_TRIANGLES, 0, 6, static_cast<GLsizei>( instances.size() ) );
        glBindVertexArray( 0 );

        if ( depthTestWasEnabled ) {
            glEnable( GL_DEPTH_TEST );
        }

        if ( !blendWasEnabled ) {
            glDisable( GL_BLEND );
        }
    }

} // namespace sloth
