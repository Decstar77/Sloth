#pragma once

#include "core/sloth_defines.h"

#include <glm/glm.hpp>

namespace sloth {

    class Font;
    class GlyphCache;
    class GuiContext;
    class GuiRenderer;
    class Input;
    class TextRenderer;

    struct GuiFrame {
        GuiContext &    ctx;
        GuiRenderer &   renderer;
        TextRenderer &  textRenderer;
        const Font &    font;
        GlyphCache &    glyphCache;
        glm::mat4       viewProjection;
    };

    GuiFrame    BeginGuiFrame( GuiContext & ctx, GuiRenderer & renderer, TextRenderer & textRenderer, const Font & font, GlyphCache & glyphCache, const Input & input, const glm::mat4 & viewProjection );
    void        EndGuiFrame( GuiFrame & frame );

} // namespace sloth
