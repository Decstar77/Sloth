#include "sloth_gui_renderer.h"

#include "renderer/sloth_shader.h"
#include "renderer/sloth_texture.h"

#include <algorithm>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

namespace sloth {
    static const char * ShapeVertexShaderSource = R"(
        #version 450 core
        layout(location = 0) in vec2 aUnit;
        layout(location = 1) in vec2 aQuadMin;
        layout(location = 2) in vec2 aQuadMax;
        layout(location = 3) in vec4 aFillColor;
        layout(location = 4) in vec4 aBorderColor;
        layout(location = 5) in float aCornerRadius;
        layout(location = 6) in float aBorderWidth;
        layout(location = 7) in vec2 aClipMin;
        layout(location = 8) in vec2 aClipMax;

        uniform mat4 uViewProjection;

        out vec2 vPixelPos;
        out vec2 vCenter;
        out vec2 vHalfSize;
        out float vCornerRadius;
        out float vBorderWidth;
        out vec4 vFillColor;
        out vec4 vBorderColor;
        out vec2 vClipMin;
        out vec2 vClipMax;

        void main()
        {
            vec2 halfSize = (aQuadMax - aQuadMin) * 0.5;

            vPixelPos = mix(aQuadMin, aQuadMax, aUnit);
            vCenter = (aQuadMin + aQuadMax) * 0.5;
            vHalfSize = halfSize;
            vCornerRadius = min(aCornerRadius, min(halfSize.x, halfSize.y));
            vBorderWidth = aBorderWidth;
            vFillColor = aFillColor;
            vBorderColor = aBorderColor;
            vClipMin = aClipMin;
            vClipMax = aClipMax;

            gl_Position = uViewProjection * vec4(vPixelPos, 0.0, 1.0);
        }
    )";

    // Evaluates each shape as a rounded-box signed distance field (Inigo
    // Quilez's sdRoundBox) rather than rasterizing geometry - a plain rect is
    // radius 0, a circle is a square quad with radius == half its size. The
    // border is a second, inset SDF evaluation of the same function so both
    // fill and stroke come from one analytic distance, antialiased against
    // its own screen-space derivative instead of MSAA/supersampling.
    static const char * ShapeFragmentShaderSource = R"(
        #version 450 core

        in vec2 vPixelPos;
        in vec2 vCenter;
        in vec2 vHalfSize;
        in float vCornerRadius;
        in float vBorderWidth;
        in vec4 vFillColor;
        in vec4 vBorderColor;
        in vec2 vClipMin;
        in vec2 vClipMax;

        out vec4 FragColor;

        float SignedDistanceRoundBox(vec2 p, vec2 halfSize, float radius)
        {
            vec2 q = abs(p) - halfSize + radius;
            return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
        }

        void main()
        {
            if (any(lessThan(vPixelPos, vClipMin)) || any(greaterThan(vPixelPos, vClipMax)))
            {
                discard;
            }

            vec2 p = vPixelPos - vCenter;
            float outerDist = SignedDistanceRoundBox(p, vHalfSize, vCornerRadius);

            float aa = max(fwidth(outerDist), 1e-4) * 0.5;
            float outerCoverage = 1.0 - smoothstep(-aa, aa, outerDist);
            if (outerCoverage <= 0.0)
            {
                discard;
            }

            vec4 color = vFillColor;
            if (vBorderWidth > 0.0)
            {
                vec2 innerHalfSize = max(vHalfSize - vBorderWidth, vec2(0.0));
                float innerRadius = max(vCornerRadius - vBorderWidth, 0.0);
                float innerDist = SignedDistanceRoundBox(p, innerHalfSize, innerRadius);
                float innerCoverage = 1.0 - smoothstep(-aa, aa, innerDist);
                color = mix(vBorderColor, vFillColor, innerCoverage);
            }

            FragColor = vec4(color.rgb, color.a * outerCoverage);
        }
    )";

    static const char * ImageVertexShaderSource = R"(
        #version 450 core
        layout(location = 0) in vec2 aUnit;
        layout(location = 1) in vec2 aQuadMin;
        layout(location = 2) in vec2 aQuadMax;
        layout(location = 3) in vec2 aUVMin;
        layout(location = 4) in vec2 aUVMax;
        layout(location = 5) in vec4 aTintColor;
        layout(location = 6) in float aCornerRadius;
        layout(location = 7) in vec2 aClipMin;
        layout(location = 8) in vec2 aClipMax;

        uniform mat4 uViewProjection;

        out vec2 vPixelPos;
        out vec2 vCenter;
        out vec2 vHalfSize;
        out vec2 vUV;
        out vec4 vTintColor;
        out float vCornerRadius;
        out vec2 vClipMin;
        out vec2 vClipMax;

        void main()
        {
            vec2 halfSize = (aQuadMax - aQuadMin) * 0.5;

            vPixelPos = mix(aQuadMin, aQuadMax, aUnit);
            vCenter = (aQuadMin + aQuadMax) * 0.5;
            vHalfSize = halfSize;
            vUV = mix(aUVMin, aUVMax, aUnit);
            vTintColor = aTintColor;
            vCornerRadius = min(aCornerRadius, min(halfSize.x, halfSize.y));
            vClipMin = aClipMin;
            vClipMax = aClipMax;

            gl_Position = uViewProjection * vec4(vPixelPos, 0.0, 1.0);
        }
    )";

    // Same rounded-box SDF clip as the shape shader (so icons/portraits can
    // be rounded or circular) layered over a plain texture sample.
    static const char * ImageFragmentShaderSource = R"(
        #version 450 core

        in vec2 vPixelPos;
        in vec2 vCenter;
        in vec2 vHalfSize;
        in vec2 vUV;
        in vec4 vTintColor;
        in float vCornerRadius;
        in vec2 vClipMin;
        in vec2 vClipMax;

        uniform sampler2D uTexture;

        out vec4 FragColor;

        float SignedDistanceRoundBox(vec2 p, vec2 halfSize, float radius)
        {
            vec2 q = abs(p) - halfSize + radius;
            return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
        }

        void main()
        {
            if (any(lessThan(vPixelPos, vClipMin)) || any(greaterThan(vPixelPos, vClipMax)))
            {
                discard;
            }

            vec2 p = vPixelPos - vCenter;
            float dist = SignedDistanceRoundBox(p, vHalfSize, vCornerRadius);

            float aa = max(fwidth(dist), 1e-4) * 0.5;
            float coverage = 1.0 - smoothstep(-aa, aa, dist);
            if (coverage <= 0.0)
            {
                discard;
            }

            vec4 texel = texture(uTexture, vUV);
            FragColor = vec4(texel.rgb * vTintColor.rgb, texel.a * vTintColor.a * coverage);
        }
    )";

    glm::mat4 MakeScreenProjection( f32 width, f32 height ) {
        return glm::ortho( 0.0f, width, height, 0.0f, -1.0f, 1.0f );
    }

    static void CreateUnitQuadVertexArray( u32 & outVertexArray, u32 & outVertexBuffer ) {
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

        glGenVertexArrays( 1, &outVertexArray );
        glBindVertexArray( outVertexArray );

        glGenBuffers( 1, &outVertexBuffer );
        glBindBuffer( GL_ARRAY_BUFFER, outVertexBuffer );
        glBufferData( GL_ARRAY_BUFFER, sizeof( unitQuad ), unitQuad, GL_STATIC_DRAW );
        glEnableVertexAttribArray( 0 );
        glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, sizeof( f32 ) * 2, reinterpret_cast<void *>( 0 ) );
    }

    GuiRenderer::GuiRenderer() {
        shapeShader = std::make_unique<Shader>( ShapeVertexShaderSource, ShapeFragmentShaderSource );

        CreateUnitQuadVertexArray( shapeVertexArray, shapeVertexBuffer );

        glGenBuffers( 1, &shapeInstanceBuffer );
        glBindBuffer( GL_ARRAY_BUFFER, shapeInstanceBuffer );

        auto shapeInstanceAttrib = []( u32 location, i32 componentCount, usize offset ) {
            glEnableVertexAttribArray( location );
            glVertexAttribPointer( location, componentCount, GL_FLOAT, GL_FALSE, sizeof( ShapeInstance ), reinterpret_cast<void *>( offset ) );
            glVertexAttribDivisor( location, 1 );
        };

        shapeInstanceAttrib( 1, 2, offsetof( ShapeInstance, QuadMin ) );
        shapeInstanceAttrib( 2, 2, offsetof( ShapeInstance, QuadMax ) );
        shapeInstanceAttrib( 3, 4, offsetof( ShapeInstance, FillColor ) );
        shapeInstanceAttrib( 4, 4, offsetof( ShapeInstance, BorderColor ) );
        shapeInstanceAttrib( 5, 1, offsetof( ShapeInstance, CornerRadius ) );
        shapeInstanceAttrib( 6, 1, offsetof( ShapeInstance, BorderWidth ) );
        shapeInstanceAttrib( 7, 2, offsetof( ShapeInstance, ClipMin ) );
        shapeInstanceAttrib( 8, 2, offsetof( ShapeInstance, ClipMax ) );

        glBindVertexArray( 0 );

        imageShader = std::make_unique<Shader>( ImageVertexShaderSource, ImageFragmentShaderSource );

        CreateUnitQuadVertexArray( imageVertexArray, imageVertexBuffer );

        glGenBuffers( 1, &imageInstanceBuffer );
        glBindBuffer( GL_ARRAY_BUFFER, imageInstanceBuffer );

        auto imageInstanceAttrib = []( u32 location, i32 componentCount, usize offset ) {
            glEnableVertexAttribArray( location );
            glVertexAttribPointer( location, componentCount, GL_FLOAT, GL_FALSE, sizeof( ImageInstance ), reinterpret_cast<void *>( offset ) );
            glVertexAttribDivisor( location, 1 );
        };

        imageInstanceAttrib( 1, 2, offsetof( ImageInstance, QuadMin ) );
        imageInstanceAttrib( 2, 2, offsetof( ImageInstance, QuadMax ) );
        imageInstanceAttrib( 3, 2, offsetof( ImageInstance, UVMin ) );
        imageInstanceAttrib( 4, 2, offsetof( ImageInstance, UVMax ) );
        imageInstanceAttrib( 5, 4, offsetof( ImageInstance, TintColor ) );
        imageInstanceAttrib( 6, 1, offsetof( ImageInstance, CornerRadius ) );
        imageInstanceAttrib( 7, 2, offsetof( ImageInstance, ClipMin ) );
        imageInstanceAttrib( 8, 2, offsetof( ImageInstance, ClipMax ) );

        glBindVertexArray( 0 );
    }

    GuiRenderer::~GuiRenderer() {
        glDeleteBuffers( 1, &imageInstanceBuffer );
        glDeleteBuffers( 1, &imageVertexBuffer );
        glDeleteVertexArrays( 1, &imageVertexArray );

        glDeleteBuffers( 1, &shapeInstanceBuffer );
        glDeleteBuffers( 1, &shapeVertexBuffer );
        glDeleteVertexArrays( 1, &shapeVertexArray );
    }

    void GuiRenderer::DrawRect( glm::vec2 min, glm::vec2 max, const glm::vec4 & color, f32 cornerRadius, f32 borderWidth, const glm::vec4 & borderColor ) {
        cornerRadius = std::max( cornerRadius, 0.0f );
        borderWidth = std::max( borderWidth, 0.0f );
        GuiRect clip = GetCurrentClipRect();
        instances.push_back( ShapeInstance { min, max, color, borderColor, cornerRadius, borderWidth, clip.min, clip.max } );
    }

    void GuiRenderer::DrawCircle( glm::vec2 center, f32 radius, const glm::vec4 & color, f32 borderWidth,
        const glm::vec4 & borderColor ) {
        DrawRect( center - glm::vec2( radius ), center + glm::vec2( radius ), color, radius, borderWidth, borderColor );
    }

    void GuiRenderer::DrawImage( glm::vec2 min, glm::vec2 max, const Texture & texture, const glm::vec4 & tintColor,
        glm::vec2 uvMin, glm::vec2 uvMax, f32 cornerRadius ) {
        cornerRadius = std::max( cornerRadius, 0.0f );
        GuiRect clip = GetCurrentClipRect();
        imageEntries.push_back( ImageDrawEntry { &texture, ImageInstance { min, max, uvMin, uvMax, tintColor, cornerRadius, clip.min, clip.max } } );
    }

    void GuiRenderer::PushClipRect( glm::vec2 min, glm::vec2 max ) {
        clipRectStack.push_back( IntersectGuiRect( GetCurrentClipRect(), GuiRect { min, max } ) );
    }

    void GuiRenderer::PopClipRect() {
        SL_ASSERT_MSG( !clipRectStack.empty(), "GuiRenderer::PopClipRect called without a matching PushClipRect" );
        clipRectStack.pop_back();
    }

    GuiRect GuiRenderer::GetCurrentClipRect() const {
        return clipRectStack.empty() ? UnboundedGuiRect() : clipRectStack.back();
    }

    void GuiRenderer::Flush( const glm::mat4 & viewProjection ) {
        SL_ASSERT_MSG( clipRectStack.empty(), "GuiRenderer::Flush: unbalanced PushClipRect/PopClipRect" );

        bool blendWasEnabled = glIsEnabled( GL_BLEND );
        bool depthTestWasEnabled = glIsEnabled( GL_DEPTH_TEST );

        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable( GL_DEPTH_TEST );

        if ( !instances.empty() ) {
            glBindBuffer( GL_ARRAY_BUFFER, shapeInstanceBuffer );
            usize requiredBytes = instances.size() * sizeof( ShapeInstance );
            if ( requiredBytes > shapeInstanceBufferCapacity ) {
                shapeInstanceBufferCapacity = requiredBytes;
                glBufferData( GL_ARRAY_BUFFER, static_cast<GLsizeiptr>( shapeInstanceBufferCapacity ), instances.data(), GL_DYNAMIC_DRAW );
            } else {
                glBufferSubData( GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>( requiredBytes ), instances.data() );
            }

            shapeShader->Bind();
            shapeShader->SetMat4( "uViewProjection", viewProjection );

            glBindVertexArray( shapeVertexArray );
            glDrawArraysInstanced( GL_TRIANGLES, 0, 6, static_cast<GLsizei>( instances.size() ) );
            glBindVertexArray( 0 );

            instances.clear();
        }

        if ( !imageEntries.empty() ) {
            imageShader->Bind();
            imageShader->SetMat4( "uViewProjection", viewProjection );
            imageShader->SetInt( "uTexture", 0 );
            FlushImages();
        }

        if ( depthTestWasEnabled ) {
            glEnable( GL_DEPTH_TEST );
        }

        if ( !blendWasEnabled ) {
            glDisable( GL_BLEND );
        }
    }

    void GuiRenderer::FlushImages() {
        glBindVertexArray( imageVertexArray );

        usize i = 0;
        while ( i < imageEntries.size() ) {
            const Texture * texture = imageEntries[i].TextureRef;

            imageUploadScratch.clear();
            while ( i < imageEntries.size() && imageEntries[i].TextureRef == texture ) {
                imageUploadScratch.push_back( imageEntries[i].Instance );
                ++i;
            }

            texture->Bind( 0 );

            glBindBuffer( GL_ARRAY_BUFFER, imageInstanceBuffer );
            usize requiredBytes = imageUploadScratch.size() * sizeof( ImageInstance );
            if ( requiredBytes > imageInstanceBufferCapacity ) {
                imageInstanceBufferCapacity = requiredBytes;
                glBufferData( GL_ARRAY_BUFFER, static_cast<GLsizeiptr>( imageInstanceBufferCapacity ), imageUploadScratch.data(), GL_DYNAMIC_DRAW );
            } else {
                glBufferSubData( GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>( requiredBytes ), imageUploadScratch.data() );
            }

            glDrawArraysInstanced( GL_TRIANGLES, 0, 6, static_cast<GLsizei>( imageUploadScratch.size() ) );
        }

        glBindVertexArray( 0 );
        imageEntries.clear();
    }

} // namespace sloth
