#pragma once

#include "core/sloth_defines.h"
#include "core/sloth_string.h"

#include <glm/glm.hpp>

namespace sloth {
    class Font;
    class GlyphCache;
    class GuiContext;
    class GuiRenderer;
    class TextRenderer;

    // First real widget wrappers built on GuiContext/GuiRenderer - the
    // hot/active/click state machine hand-rolled in the sandbox demo,
    // extracted once so callers stop re-deriving it per call site. No
    // GuiStyle exists yet (see the GUI system roadmap, item 12), so colors
    // and sizing are hardcoded constants in the .cpp rather than parameters
    // - expect these signatures to shrink once a style stack lands.
    //
    // Font/GlyphCache/TextRenderer are threaded through explicitly rather
    // than folded into GuiContext, matching how GuiRenderer/TextRenderer are
    // already separate, explicitly-owned objects with no hidden globals.
    // Widgets that draw text also need `viewProjection` because
    // TextRenderer::DrawText issues its own immediate draw call (unlike
    // GuiRenderer's queued shapes) - Button/Checkbox internally Flush()
    // `renderer` before drawing their label so the label lands on top of
    // the background rect regardless of what the caller queued earlier this
    // frame; see GuiRenderer's class comment for why shape/text ordering
    // isn't automatic yet.

    // A clickable rect. `label` follows the "Save##File" convention
    // (see StripGuiLabelHash) - everything before "##" is drawn, the whole
    // string (including the suffix) is hashed for the widget's GuiId, so
    // two buttons that must display the same text can still be
    // disambiguated. Returns true on the frame the button is released while
    // still hovered (a "click"), not on press.
    bool Button( GuiContext & ctx, GuiRenderer & renderer, TextRenderer & textRenderer, const Font & font,
        GlyphCache & glyphCache, StringView label, glm::vec2 min, glm::vec2 max, const glm::mat4 & viewProjection );

    // A labeled toggle box. Toggles `value` in place when clicked and
    // returns true on the frame it changed - `value` itself lives in caller
    // state, not GuiStorage, since there's no reason to duplicate it.
    // `pos` is the box's top-left corner; the label is drawn to its right.
    bool Checkbox( GuiContext & ctx, GuiRenderer & renderer, TextRenderer & textRenderer, const Font & font,
        GlyphCache & glyphCache, StringView label, glm::vec2 pos, bool & value, const glm::mat4 & viewProjection );

    // Thin wrapper around TextRenderer::DrawText - not interactive, has no
    // GuiId/hot-active state of its own. Exists mainly for API symmetry
    // with Button/Checkbox today, and as the hook a future auto-layout
    // cursor (roadmap item 7) will drive instead of a caller-supplied
    // baseline position.
    void Label( TextRenderer & textRenderer, const Font & font, GlyphCache & glyphCache, StringView text,
        glm::vec2 baselinePos, f32 pixelHeight, const glm::vec4 & color, const glm::mat4 & viewProjection );

} // namespace sloth
