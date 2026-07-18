#pragma once

#include "core/sloth_defines.h"
#include "core/sloth_string.h"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace sloth {
    class Font;
    class GlyphCache;
    class Shader;

    // Draws text by batching one instanced quad per glyph and evaluating
    // each glyph's curves directly in the fragment shader (the Slug
    // technique) - no bitmap atlas, crisp at any scale. This first pass
    // loops over every curve in the glyph per pixel (no band acceleration),
    // which is fine for typical Latin glyph curve counts.
    class TextRenderer {
      public:
        TextRenderer();
        ~TextRenderer();

        SL_NON_COPYABLE( TextRenderer );
        SL_NON_MOVABLE( TextRenderer );

        // Draws `text` (ASCII only - no UTF-8 decoding yet) with its
        // baseline starting at `baselinePos` (pixels, y-down), `pixelHeight`
        // font pixels tall. One batched instanced draw call per invocation.
        // Manages its own GL state (blending on, depth test/write off)
        // around the draw.
        void DrawText( const Font & font, GlyphCache & cache, StringView text, glm::vec2 baselinePos, f32 pixelHeight,
            const glm::vec4 & color, const glm::mat4 & viewProjection );

      private:
        struct GlyphInstance {
            glm::vec2 QuadMin;
            glm::vec2 QuadMax;
            glm::vec2 CurveMin;
            glm::vec2 CurveMax;
            u32 CurveOffset;
            u32 CurveCount;
            glm::vec4 Color;
        };

      private:
        std::unique_ptr<Shader> shader;

        u32 quadVertexArray = 0;
        u32 quadVertexBuffer = 0;
        u32 instanceBuffer = 0;
        usize instanceBufferCapacity = 0;

        std::vector<GlyphInstance> instances; // Scratch, rebuilt each DrawText call.
    };

} // namespace sloth
