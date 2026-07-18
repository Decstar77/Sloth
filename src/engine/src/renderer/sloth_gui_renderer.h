#pragma once

#include "core/sloth_defines.h"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace sloth
{
    class Shader;

    // Builds an orthographic projection for screen-space 2D rendering: origin
    // at the top-left, x right, y down (pixel coordinates), matching the
    // y-down convention TextRenderer already uses for baselinePos.
    glm::mat4 MakeScreenProjection(f32 width, f32 height);

    // Batches flat-colored, screen-space rectangles (the GUI system's base
    // primitive) into one instanced draw call per Flush(). Queue rects with
    // DrawRect() in any order during the frame, then call Flush() once -
    // mirrors TextRenderer's per-call batching, just without a glyph/curve
    // texture. Manages its own GL state (blending on, depth test/write off)
    // around the draw, like TextRenderer does.
    class GuiRenderer
    {
    public:
        GuiRenderer();
        ~GuiRenderer();

        SL_NON_COPYABLE(GuiRenderer);
        SL_NON_MOVABLE(GuiRenderer);

        // Queues one solid rectangle, `min`/`max` in screen pixels (y-down).
        void DrawRect(glm::vec2 min, glm::vec2 max, const glm::vec4& color);

        // Draws every rect queued since the last Flush() in a single batched
        // instanced draw call, then clears the queue.
        void Flush(const glm::mat4& viewProjection);

    private:
        struct RectInstance
        {
            glm::vec2 QuadMin;
            glm::vec2 QuadMax;
            glm::vec4 Color;
        };

    private:
        std::unique_ptr<Shader> shader;

        u32 quadVertexArray = 0;
        u32 quadVertexBuffer = 0;
        u32 instanceBuffer = 0;
        usize instanceBufferCapacity = 0;

        std::vector<RectInstance> instances; // Scratch, rebuilt each frame.
    };

} // namespace sloth
