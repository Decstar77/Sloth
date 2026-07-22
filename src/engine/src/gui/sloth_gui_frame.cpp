#include "sloth_gui_frame.h"

#include "gui/sloth_gui_context.h"

namespace sloth {

    GuiFrame BeginGuiFrame( GuiContext & ctx, GuiRenderer & renderer, TextRenderer & textRenderer, const Font & font, GlyphCache & glyphCache, const Input & input, const glm::mat4 & viewProjection ) {
        ctx.NewFrame( input );
        return GuiFrame { ctx, renderer, textRenderer, font, glyphCache, viewProjection };
    }

    void EndGuiFrame( GuiFrame & frame ) {
        frame.ctx.EndFrame();
    }

} // namespace sloth
