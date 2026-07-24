#include "sloth_gui_widgets.h"

#include "audio/sloth_audio_world.h"
#include "font/sloth_font.h"
#include "gui/sloth_gui_context.h"
#include "renderer/sloth_gui_renderer.h"
#include "renderer/sloth_text_renderer.h"

namespace sloth {

    namespace {
        const char * ButtonHoverSoundPath = "../../assets/sounds/button_hover.wav";
        const char * ButtonClickSoundPath = "../../assets/sounds/button_click.wav";

        constexpr f32 ButtonCornerRadius = 8.0f;
        constexpr f32 ButtonTextPixelHeight = 16.0f;
        constexpr f32 ButtonTextPaddingX = 12.0f;
        const glm::vec4 ButtonColorNormal{ 0.24f, 0.24f, 0.28f, 1.0f };
        const glm::vec4 ButtonColorHot{ 0.32f, 0.32f, 0.38f, 1.0f };
        const glm::vec4 ButtonColorActive{ 0.18f, 0.18f, 0.22f, 1.0f };
        const glm::vec4 ButtonTextColor{ 1.0f, 1.0f, 1.0f, 1.0f };

        constexpr f32 CheckboxBoxSize = 20.0f;
        constexpr f32 CheckboxCornerRadius = 4.0f;
        constexpr f32 CheckboxBorderWidth = 1.5f;
        constexpr f32 CheckboxMarkInset = 5.0f;
        constexpr f32 CheckboxMarkCornerRadius = 2.0f;
        constexpr f32 CheckboxLabelGap = 8.0f;
        constexpr f32 CheckboxTextPixelHeight = 16.0f;
        const glm::vec4 CheckboxColorNormal{ 0.18f, 0.18f, 0.22f, 1.0f };
        const glm::vec4 CheckboxColorHot{ 0.24f, 0.24f, 0.28f, 1.0f };
        const glm::vec4 CheckboxColorActive{ 0.14f, 0.14f, 0.18f, 1.0f };
        const glm::vec4 CheckboxBorderColor{ 0.5f, 0.5f, 0.55f, 1.0f };
        const glm::vec4 CheckboxMarkColor{ 0.3f, 0.65f, 1.0f, 1.0f };
        const glm::vec4 CheckboxTextColor{ 1.0f, 1.0f, 1.0f, 1.0f };

        // Rough vertical-centering offset from a rect's vertical midpoint to
        // a text baseline, since Font doesn't expose per-call ascent lookup
        // through TextRenderer yet - good enough for the default sizes above,
        // will want revisiting once real ascent/descent metrics are threaded
        // through (or once GuiStyle standardizes text sizes).
        constexpr f32 TextBaselineMidpointOffset = 5.0f;

        constexpr f32 PanelCornerRadius = 12.0f;
        constexpr f32 PanelTitleBarHeight = 28.0f;
        constexpr f32 PanelContentPadding = 12.0f;
        constexpr f32 PanelTitleTextPixelHeight = 16.0f;
        constexpr f32 PanelTitleTextPaddingX = 12.0f;
        const glm::vec4 PanelBodyColor{ 0.10f, 0.10f, 0.13f, 0.94f };
        const glm::vec4 PanelTitleColorNormal{ 0.18f, 0.18f, 0.22f, 1.0f };
        const glm::vec4 PanelTitleColorHot{ 0.24f, 0.24f, 0.30f, 1.0f };
        const glm::vec4 PanelTitleColorActive{ 0.28f, 0.30f, 0.40f, 1.0f };
        const glm::vec4 PanelTitleTextColor{ 0.92f, 0.92f, 0.95f, 1.0f };
    } // namespace

    bool Button( GuiFrame & frame, StringView label, glm::vec2 min, glm::vec2 max ) {
        GuiContext & ctx = frame.ctx;
        GuiId id = ctx.GetId( label );

        bool hovered = GuiContext::IsPointInRect( ctx.GetMousePos(), min, max ) && ctx.IsPointVisible( ctx.GetMousePos() );
        if ( hovered ) {
            ctx.SetHot( id );
        }

        if ( ctx.IsHot( id ) && ctx.IsMousePressed() ) {
            ctx.SetActive( id );
            ctx.SetFocused( id );
        }

        bool clicked = false;
        if ( ctx.IsActive( id ) && ctx.IsMouseReleased() ) {
            clicked = ctx.IsHot( id );
            ctx.ClearActive();
        }

        // Hover sound fires only on the not-hovered -> hovered transition
        // (tracked per-id in GuiStorage), not every frame the mouse sits
        // over the button.
        if ( frame.audioWorld != nullptr ) {
            bool & wasHovered = ctx.GetStorage().GetOrAddBool( HashGuiId( "##hoverSound", id ) );
            if ( hovered && !wasHovered ) {
                frame.audioWorld->PlaySound2D( ButtonHoverSoundPath );
            }
            wasHovered = hovered;

            if ( clicked ) {
                frame.audioWorld->PlaySound2D( ButtonClickSoundPath );
            }
        }

        glm::vec4 color = ctx.IsActive( id ) ? ButtonColorActive : ctx.IsHot( id ) ? ButtonColorHot : ButtonColorNormal;
        frame.renderer.DrawRect( min, max, color, ButtonCornerRadius );
        frame.renderer.Flush( frame.viewProjection );

        if ( frame.font.IsLoaded() ) {
            StringView displayLabel = StripGuiLabelHash( label );
            glm::vec2 textPos{ min.x + ButtonTextPaddingX, ( min.y + max.y ) * 0.5f + TextBaselineMidpointOffset };
            frame.textRenderer.DrawText( frame.font, frame.glyphCache, displayLabel, textPos, ButtonTextPixelHeight, ButtonTextColor, frame.viewProjection );
        }

        return clicked;
    }

    bool Checkbox( GuiFrame & frame, StringView label, glm::vec2 pos, bool & value ) {
        GuiContext & ctx = frame.ctx;
        GuiId id = ctx.GetId( label );
        glm::vec2 boxMin = pos;
        glm::vec2 boxMax = pos + glm::vec2( CheckboxBoxSize, CheckboxBoxSize );

        bool hovered = GuiContext::IsPointInRect( ctx.GetMousePos(), boxMin, boxMax ) && ctx.IsPointVisible( ctx.GetMousePos() );
        if ( hovered ) {
            ctx.SetHot( id );
        }

        if ( ctx.IsHot( id ) && ctx.IsMousePressed() ) {
            ctx.SetActive( id );
            ctx.SetFocused( id );
        }

        bool changed = false;
        if ( ctx.IsActive( id ) && ctx.IsMouseReleased() ) {
            if ( ctx.IsHot( id ) ) {
                value = !value;
                changed = true;
            }
            ctx.ClearActive();
        }

        glm::vec4 boxColor = ctx.IsActive( id ) ? CheckboxColorActive : ctx.IsHot( id ) ? CheckboxColorHot : CheckboxColorNormal;
        frame.renderer.DrawRect( boxMin, boxMax, boxColor, CheckboxCornerRadius, CheckboxBorderWidth, CheckboxBorderColor );
        if ( value ) {
            glm::vec2 markMin = boxMin + glm::vec2( CheckboxMarkInset, CheckboxMarkInset );
            glm::vec2 markMax = boxMax - glm::vec2( CheckboxMarkInset, CheckboxMarkInset );
            frame.renderer.DrawRect( markMin, markMax, CheckboxMarkColor, CheckboxMarkCornerRadius );
        }
        frame.renderer.Flush( frame.viewProjection );

        if ( frame.font.IsLoaded() ) {
            StringView displayLabel = StripGuiLabelHash( label );
            glm::vec2 textPos{ boxMax.x + CheckboxLabelGap, ( boxMin.y + boxMax.y ) * 0.5f + TextBaselineMidpointOffset };
            frame.textRenderer.DrawText( frame.font, frame.glyphCache, displayLabel, textPos, CheckboxTextPixelHeight, CheckboxTextColor, frame.viewProjection );
        }

        return changed;
    }

    void Label( GuiFrame & frame, StringView text, glm::vec2 baselinePos, f32 pixelHeight, const glm::vec4 & color ) {
        if ( !frame.font.IsLoaded() ) {
            return;
        }
        frame.textRenderer.DrawText( frame.font, frame.glyphCache, text, baselinePos, pixelHeight, color, frame.viewProjection );
    }

    PanelResult BeginPanel( GuiFrame & frame, StringView label, glm::vec2 defaultPos, glm::vec2 size, bool draggable ) {
        GuiContext & ctx = frame.ctx;
        GuiId id = ctx.GetId( label );

        // Scope every child widget's id to this panel, so two panels whose
        // content happens to produce identical child labels (e.g. two
        // inventory-grid panels both listing the same underlying Inventory)
        // don't collide onto the same GuiId and cross-highlight each other.
        // Balanced by EndPanel()'s PopId().
        ctx.PushId( label );

        // Position is the one piece of panel state that must outlive the
        // frame: a dragged panel stays where the user left it. `defaultPos`
        // only seeds the slot the first frame this ID is seen.
        glm::vec2 & pos = ctx.GetStorage().GetOrAddVec2( id, defaultPos );

        glm::vec2 titleMin = pos;
        glm::vec2 titleMax = pos + glm::vec2( size.x, PanelTitleBarHeight );

        // Hovering anywhere over the whole panel claims hot so gameplay code
        // (via WantsMouseInput) won't also react to the same click. A child
        // widget drawn later overrides this - last SetHot() in a frame wins -
        // so this only captures input over the panel's empty regions.
        glm::vec2 panelMin = pos;
        glm::vec2 panelMax = pos + size;
        bool overPanel = GuiContext::IsPointInRect( ctx.GetMousePos(), panelMin, panelMax ) && ctx.IsPointVisible( ctx.GetMousePos() );
        if ( overPanel ) {
            ctx.SetHot( id );
        }

        // Dragging is grabbed on the title bar only, but a live drag stays
        // active regardless of where the pointer wanders (activeId locks out
        // every other widget's hot claim until release), so a fast drag can't
        // "slip off" the bar.
        if ( draggable ) {
            // Offset from the panel origin to the mouse at grab time, kept in
            // GuiStorage so the panel doesn't snap its corner to the cursor.
            GuiId dragId = HashGuiId( "##drag", id );
            glm::vec2 & dragAnchor = ctx.GetStorage().GetOrAddVec2( dragId );

            bool overTitle = GuiContext::IsPointInRect( ctx.GetMousePos(), titleMin, titleMax ) && ctx.IsPointVisible( ctx.GetMousePos() );
            if ( ctx.IsHot( id ) && overTitle && ctx.IsMousePressed() ) {
                ctx.SetActive( id );
                ctx.SetFocused( id );
                dragAnchor = ctx.GetMousePos() - pos;
            }

            if ( ctx.IsActive( id ) ) {
                if ( ctx.IsMouseDown() ) {
                    pos = ctx.GetMousePos() - dragAnchor;
                } else {
                    ctx.ClearActive();
                }

                // Re-derive rects after the move so this frame draws at the
                // dragged-to position rather than lagging one frame behind.
                titleMin = pos;
                titleMax = pos + glm::vec2( size.x, PanelTitleBarHeight );
                panelMin = pos;
                panelMax = pos + size;
            }
        }

        // Body first, then the title bar over its top so the shared rounded
        // corners read as one panel. Flush immediately: content drawn after
        // this (and TextRenderer, which draws immediately rather than queuing)
        // must land on top - the same ordering workaround Button/Checkbox use
        // until draw layers exist (roadmap item 8).
        frame.renderer.DrawRect( panelMin, panelMax, PanelBodyColor, PanelCornerRadius );
        glm::vec4 titleColor = ctx.IsActive( id ) ? PanelTitleColorActive : ( draggable && ctx.IsHot( id ) ) ? PanelTitleColorHot : PanelTitleColorNormal;
        frame.renderer.DrawRect( titleMin, titleMax, titleColor, PanelCornerRadius );
        // Square off the title bar's lower corners against the body.
        frame.renderer.DrawRect( { titleMin.x, ( titleMin.y + titleMax.y ) * 0.5f }, titleMax, titleColor, 0.0f );
        frame.renderer.Flush( frame.viewProjection );

        if ( frame.font.IsLoaded() ) {
            StringView displayLabel = StripGuiLabelHash( label );
            glm::vec2 textPos{ titleMin.x + PanelTitleTextPaddingX, ( titleMin.y + titleMax.y ) * 0.5f + TextBaselineMidpointOffset };
            frame.textRenderer.DrawText( frame.font, frame.glyphCache, displayLabel, textPos, PanelTitleTextPixelHeight, PanelTitleTextColor, frame.viewProjection );
        }

        PanelResult result;
        result.contentMin = glm::vec2( panelMin.x + PanelContentPadding, titleMax.y + PanelContentPadding );
        result.contentMax = glm::vec2( panelMax.x - PanelContentPadding, panelMax.y - PanelContentPadding );

        // Clip content to the body's interior on both stacks: renderer clips
        // per-fragment, ctx gates hit-testing, kept in lockstep so an
        // overflowing widget is neither drawn nor clickable past the edge.
        glm::vec2 clipMin{ panelMin.x, titleMax.y };
        glm::vec2 clipMax = panelMax;
        frame.renderer.PushClipRect( clipMin, clipMax );
        ctx.PushClipRect( clipMin, clipMax );

        return result;
    }

    void EndPanel( GuiFrame & frame ) {
        // Flush before popping so any content the caller queued (raw DrawRect
        // etc.) is drawn while the panel's clip rect is still on the stack;
        // otherwise it would inherit the wrong clip after the pop.
        frame.renderer.Flush( frame.viewProjection );
        frame.renderer.PopClipRect();
        frame.ctx.PopClipRect();
        frame.ctx.PopId();
    }

} // namespace sloth
