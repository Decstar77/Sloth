#pragma once

#include "core/sloth_defines.h"
#include "renderer/sloth_gui_rect.h"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace sloth {
    class Shader;
    class Texture;

    // Builds an orthographic projection for screen-space 2D rendering: origin
    // at the top-left, x right, y down (pixel coordinates), matching the
    // y-down convention TextRenderer already uses for baselinePos.
    glm::mat4 MakeScreenProjection( f32 width, f32 height );

    // Batches screen-space shapes and images (the GUI system's base
    // primitives) into instanced draw calls issued from Flush(). Solid
    // shapes are a rounded box evaluated as a signed-distance field in the
    // fragment shader - resolution-independent edges/corners/borders with no
    // texture atlas, and a circle is just the degenerate case where
    // cornerRadius == half the shape's size. Images share the same rounded-
    // box clipping so icons can have rounded/circular corners too, but are
    // batched separately since each draw call can only bind one texture:
    // Flush() groups consecutive DrawImage() calls that share a texture into
    // one instanced draw, and issues a new one whenever the texture changes.
    //
    // Flush() always draws the shape batch first, then the image batch, in
    // each case preserving call order within that batch - so an image drawn
    // "on top of" a rect via interleaved calls only works if all rects were
    // queued before any interleaving image; true call-order interleaving
    // between shapes and images needs a layered draw list, which doesn't
    // exist yet (see the GUI system roadmap).
    class GuiRenderer {
      public:
        GuiRenderer();
        ~GuiRenderer();

        SL_NON_COPYABLE( GuiRenderer );
        SL_NON_MOVABLE( GuiRenderer );

        // Queues one rectangle, `min`/`max` in screen pixels (y-down).
        // `cornerRadius` is clamped to half the shape's shorter side (so it
        // can't overshoot into a bowtie). If `borderWidth` > 0, a
        // `borderColor` ring of that thickness is drawn inset from the edge,
        // with `color` filling the interior - pass a transparent `color` for
        // an outline-only rect.
        void DrawRect( glm::vec2 min, glm::vec2 max, const glm::vec4 & color, f32 cornerRadius = 0.0f,
            f32 borderWidth = 0.0f, const glm::vec4 & borderColor = glm::vec4( 0.0f ) );

        // Queues one circle at `center` with the given `radius`. Same
        // border behavior as DrawRect().
        void DrawCircle( glm::vec2 center, f32 radius, const glm::vec4 & color, f32 borderWidth = 0.0f,
            const glm::vec4 & borderColor = glm::vec4( 0.0f ) );

        // Queues one textured rectangle, `min`/`max` in screen pixels
        // (y-down), sampling `texture` over `uvMin`/`uvMax` (default: the
        // whole image) and multiplying by `tintColor` (default: opaque
        // white, i.e. unmodified). `cornerRadius` clips the image to a
        // rounded/circular shape same as DrawRect(). `texture` must outlive
        // the following Flush() call.
        void DrawImage( glm::vec2 min, glm::vec2 max, const Texture & texture, const glm::vec4 & tintColor = glm::vec4( 1.0f ),
            glm::vec2 uvMin = glm::vec2( 0.0f ), glm::vec2 uvMax = glm::vec2( 1.0f ), f32 cornerRadius = 0.0f );

        // Narrows the active clip rect (intersected with whatever was
        // already pushed) that every DrawRect()/DrawCircle()/DrawImage()
        // call queues into its instance data from this point until the
        // matching PopClipRect(). Unlike a GL scissor test, this costs
        // nothing extra per draw call and needs no batch break - each
        // instance just carries its own clip rect and gets discarded
        // per-fragment in the same shader pass that already evaluates its
        // SDF, so differently-clipped shapes can freely share one batch.
        // The clip stack is persistent state independent of Flush() - it may
        // be non-empty across a Flush (e.g. a widget flushing its own
        // background inside a pushed panel clip); just balance each
        // PushClipRect() with a PopClipRect() within the frame.
        void PushClipRect( glm::vec2 min, glm::vec2 max );
        void PopClipRect();

        // Draws every shape and image queued since the last Flush(), then
        // clears both queues. See the class comment for draw-order caveats.
        void Flush( const glm::mat4 & viewProjection );

      private:
        struct ShapeInstance {
            glm::vec2 QuadMin;
            glm::vec2 QuadMax;
            glm::vec4 FillColor;
            glm::vec4 BorderColor;
            f32 CornerRadius;
            f32 BorderWidth;
            glm::vec2 ClipMin;
            glm::vec2 ClipMax;
        };

        struct ImageInstance {
            glm::vec2 QuadMin;
            glm::vec2 QuadMax;
            glm::vec2 UVMin;
            glm::vec2 UVMax;
            glm::vec4 TintColor;
            f32 CornerRadius;
            glm::vec2 ClipMin;
            glm::vec2 ClipMax;
        };

        struct ImageDrawEntry {
            const Texture * TextureRef;
            ImageInstance Instance;
        };

      private:
        void FlushImages();
        GuiRect GetCurrentClipRect() const;

      private:
        std::unique_ptr<Shader> shapeShader;
        u32 shapeVertexArray = 0;
        u32 shapeVertexBuffer = 0;
        u32 shapeInstanceBuffer = 0;
        usize shapeInstanceBufferCapacity = 0;
        std::vector<ShapeInstance> instances; // Scratch, rebuilt each frame.

        std::unique_ptr<Shader> imageShader;
        u32 imageVertexArray = 0;
        u32 imageVertexBuffer = 0;
        u32 imageInstanceBuffer = 0;
        usize imageInstanceBufferCapacity = 0;
        std::vector<ImageDrawEntry> imageEntries;      // Scratch, rebuilt each frame.
        std::vector<ImageInstance> imageUploadScratch; // Scratch, one texture-run at a time.

        std::vector<GuiRect> clipRectStack;
    };

} // namespace sloth
