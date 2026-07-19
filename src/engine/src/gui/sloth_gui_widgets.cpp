#include "sloth_gui_widgets.h"

#include "font/sloth_font.h"
#include "gui/sloth_gui_context.h"
#include "renderer/sloth_gui_renderer.h"
#include "renderer/sloth_text_renderer.h"

namespace sloth {

    namespace {
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
    } // namespace

    bool Button( GuiContext & ctx, GuiRenderer & renderer, TextRenderer & textRenderer, const Font & font,
        GlyphCache & glyphCache, StringView label, glm::vec2 min, glm::vec2 max, const glm::mat4 & viewProjection ) {
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

        glm::vec4 color = ctx.IsActive( id ) ? ButtonColorActive : ctx.IsHot( id ) ? ButtonColorHot : ButtonColorNormal;
        renderer.DrawRect( min, max, color, ButtonCornerRadius );
        renderer.Flush( viewProjection );

        if ( font.IsLoaded() ) {
            StringView displayLabel = StripGuiLabelHash( label );
            glm::vec2 textPos{ min.x + ButtonTextPaddingX, ( min.y + max.y ) * 0.5f + TextBaselineMidpointOffset };
            textRenderer.DrawText( font, glyphCache, displayLabel, textPos, ButtonTextPixelHeight, ButtonTextColor, viewProjection );
        }

        return clicked;
    }

    bool Checkbox( GuiContext & ctx, GuiRenderer & renderer, TextRenderer & textRenderer, const Font & font,
        GlyphCache & glyphCache, StringView label, glm::vec2 pos, bool & value, const glm::mat4 & viewProjection ) {
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
        renderer.DrawRect( boxMin, boxMax, boxColor, CheckboxCornerRadius, CheckboxBorderWidth, CheckboxBorderColor );
        if ( value ) {
            glm::vec2 markMin = boxMin + glm::vec2( CheckboxMarkInset, CheckboxMarkInset );
            glm::vec2 markMax = boxMax - glm::vec2( CheckboxMarkInset, CheckboxMarkInset );
            renderer.DrawRect( markMin, markMax, CheckboxMarkColor, CheckboxMarkCornerRadius );
        }
        renderer.Flush( viewProjection );

        if ( font.IsLoaded() ) {
            StringView displayLabel = StripGuiLabelHash( label );
            glm::vec2 textPos{ boxMax.x + CheckboxLabelGap, ( boxMin.y + boxMax.y ) * 0.5f + TextBaselineMidpointOffset };
            textRenderer.DrawText( font, glyphCache, displayLabel, textPos, CheckboxTextPixelHeight, CheckboxTextColor, viewProjection );
        }

        return changed;
    }

    void Label( TextRenderer & textRenderer, const Font & font, GlyphCache & glyphCache, StringView text, glm::vec2 baselinePos,
        f32 pixelHeight, const glm::vec4 & color, const glm::mat4 & viewProjection ) {
        if ( !font.IsLoaded() ) {
            return;
        }
        textRenderer.DrawText( font, glyphCache, text, baselinePos, pixelHeight, color, viewProjection );
    }

} // namespace sloth
