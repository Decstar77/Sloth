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

    // Bundles everything a widget call needs to draw and hit-test for one
    // frame - the GuiContext itself plus the renderer/text/font objects and
    // view-projection matrix every widget call previously took as five-plus
    // separate parameters. Built once per frame by BeginGuiFrame(), passed
    // by reference to Button/Label/BeginPanel/etc. Holds references to
    // objects owned elsewhere, not by value - it's a per-frame view, not a
    // container.
    struct GuiFrame {
        GuiContext &    ctx;
        GuiRenderer &   renderer;
        TextRenderer &  textRenderer;
        const Font &    font;
        GlyphCache &    glyphCache;
        glm::mat4       viewProjection;
    };

    // Starts a GUI frame: advances GuiContext's per-frame input snapshot
    // (GuiContext::NewFrame) and packages the rendering dependencies every
    // widget call needs into one GuiFrame. Call once per frame before any
    // widget calls; pair with EndGuiFrame() after the last one.
    GuiFrame BeginGuiFrame( GuiContext & ctx, GuiRenderer & renderer, TextRenderer & textRenderer, const Font & font,
        GlyphCache & glyphCache, const Input & input, const glm::mat4 & viewProjection );

    // Closes the frame opened by BeginGuiFrame() - calls GuiContext::EndFrame(),
    // asserting the PushId/PopId and clip-rect stacks balanced this frame.
    void EndGuiFrame( GuiFrame & frame );

} // namespace sloth
