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

    // What BeginPanel() hands back so the caller knows where it may place
    // content: the clipped content rect (title bar excluded, padding
    // applied), in screen pixels (y-down). Widgets drawn between BeginPanel
    // and EndPanel should position themselves relative to `contentMin`; any
    // part spilling past `contentMax` is clipped away (and its clipped-off
    // region is not hoverable) by the clip rect BeginPanel pushed.
    struct PanelResult {
        glm::vec2 contentMin;
        glm::vec2 contentMax;
    };

    // Begins a draggable, clippable container. Must be paired with a matching
    // EndPanel() later the same frame (like PushClipRect/PopClipRect). The
    // panel's top-left position is stored in GuiStorage keyed by the panel's
    // GuiId, so it survives across frames once dragged - `defaultPos` only
    // seeds it the first frame this panel's ID is seen. `size` is the panel's
    // full extent (title bar + content). `label` follows the "Save##File"
    // convention and is drawn in the title bar.
    //
    // When `draggable`, grabbing the title bar and moving the mouse repositions
    // the panel (position-drag only; no resize handles yet - roadmap 6b).
    // Hovering anywhere over the panel claims hot (WantsMouseInput), so gameplay
    // code below the GUI won't also react to a click that lands on the panel.
    //
    // BeginPanel draws and Flush()es its own background immediately, then pushes
    // a content clip rect on both `renderer` and `ctx`; EndPanel pops them. Draw
    // panel content between the two calls.
    PanelResult BeginPanel( GuiContext & ctx, GuiRenderer & renderer, TextRenderer & textRenderer, const Font & font,
        GlyphCache & glyphCache, StringView label, glm::vec2 defaultPos, glm::vec2 size, const glm::mat4 & viewProjection,
        bool draggable = true );

    // Closes the panel opened by the matching BeginPanel(). Flushes any content
    // the caller queued into `renderer` (so it's drawn while the panel's clip
    // rect is still active) and pops the clip rect from both `renderer` and
    // `ctx`.
    void EndPanel( GuiContext & ctx, GuiRenderer & renderer, const glm::mat4 & viewProjection );

} // namespace sloth
