#pragma once

#include "core/sloth_defines.h"
#include "renderer/sloth_gui_rect.h"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace sloth {
    class Shader;
    class Texture;

    glm::mat4 MakeScreenProjection( f32 width, f32 height );

    class GuiRenderer {
      public:
        GuiRenderer();
        ~GuiRenderer();

        SL_NON_COPYABLE( GuiRenderer );
        SL_NON_MOVABLE( GuiRenderer );

        void DrawRect( glm::vec2 min, glm::vec2 max, const glm::vec4 & color, f32 cornerRadius = 0.0f, f32 borderWidth = 0.0f, const glm::vec4 & borderColor = glm::vec4( 0.0f ) );
        void DrawCircle( glm::vec2 center, f32 radius, const glm::vec4 & color, f32 borderWidth = 0.0f, const glm::vec4 & borderColor = glm::vec4( 0.0f ) );
        void DrawImage( glm::vec2 min, glm::vec2 max, const Texture & texture, const glm::vec4 & tintColor = glm::vec4( 1.0f ), glm::vec2 uvMin = glm::vec2( 0.0f ), glm::vec2 uvMax = glm::vec2( 1.0f ), f32 cornerRadius = 0.0f );

        void PushClipRect( glm::vec2 min, glm::vec2 max );
        void PopClipRect();

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
