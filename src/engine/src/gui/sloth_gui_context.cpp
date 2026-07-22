#include "sloth_gui_context.h"

#include "core/sloth_input.h"

namespace sloth {
    bool & GuiStorage::GetOrAddBool( GuiId id, bool defaultValue ) {
        auto [it, inserted] = values.try_emplace( id );
        if ( inserted ) {
            it->second.AsBool = defaultValue;
        }
        return it->second.AsBool;
    }

    i32 & GuiStorage::GetOrAddInt( GuiId id, i32 defaultValue ) {
        auto [it, inserted] = values.try_emplace( id );
        if ( inserted ) {
            it->second.AsInt = defaultValue;
        }
        return it->second.AsInt;
    }

    f32 & GuiStorage::GetOrAddFloat( GuiId id, f32 defaultValue ) {
        auto [it, inserted] = values.try_emplace( id );
        if ( inserted ) {
            it->second.AsFloat = defaultValue;
        }
        return it->second.AsFloat;
    }

    glm::vec2 & GuiStorage::GetOrAddVec2( GuiId id, glm::vec2 defaultValue ) {
        auto [it, inserted] = values.try_emplace( id );
        if ( inserted ) {
            it->second.AsVec2 = defaultValue;
        }
        return it->second.AsVec2;
    }

    void GuiStorage::Clear() {
        values.clear();
    }

    void GuiContext::NewFrame( const Input & input ) {
        SL_ASSERT_MSG( idStack.size() == 1, "GuiContext::NewFrame: unbalanced PushId/PopId left over from last frame" );
        SL_ASSERT_MSG( clipRectStack.empty(), "GuiContext::NewFrame: unbalanced PushClipRect/PopClipRect left over from last frame" );

        hotId = InvalidGuiId;

        mousePos = glm::vec2( static_cast<f32>( input.GetMouseX() ), static_cast<f32>( input.GetMouseY() ) );
        mouseDown = input.IsMouseButtonDown( MouseButton::Left );
        mousePressed = input.IsMouseButtonPressed( MouseButton::Left );
        mouseReleased = input.IsMouseButtonReleased( MouseButton::Left );
    }

    void GuiContext::EndFrame() {
        SL_ASSERT_MSG( idStack.size() == 1, "GuiContext::EndFrame: unbalanced PushId/PopId (missing a PopId somewhere this frame)" );
        SL_ASSERT_MSG( clipRectStack.empty(), "GuiContext::EndFrame: unbalanced PushClipRect/PopClipRect (missing a PopClipRect somewhere this frame)" );
    }

    void GuiContext::PushId( StringView label ) {
        idStack.push_back( HashGuiId( label, idStack.back() ) );
    }

    void GuiContext::PushId( i32 value ) {
        idStack.push_back( HashGuiId( value, idStack.back() ) );
    }

    void GuiContext::PopId() {
        SL_ASSERT_MSG( idStack.size() > 1, "GuiContext::PopId called without a matching PushId" );
        idStack.pop_back();
    }

    GuiId GuiContext::GetId( StringView label ) const {
        return HashGuiId( label, idStack.back() );
    }

    GuiId GuiContext::GetId( i32 value ) const {
        return HashGuiId( value, idStack.back() );
    }

    void GuiContext::SetHot( GuiId id ) {
        if ( activeId == InvalidGuiId || activeId == id ) {
            hotId = id;
        }
    }

    void GuiContext::SetActive( GuiId id ) {
        activeId = id;
    }

    void GuiContext::ClearActive() {
        activeId = InvalidGuiId;
    }

    void GuiContext::SetFocused( GuiId id ) {
        focusedId = id;
    }

    void GuiContext::ClearFocused() {
        focusedId = InvalidGuiId;
    }

    bool GuiContext::IsPointInRect( glm::vec2 point, glm::vec2 min, glm::vec2 max ) {
        return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
    }

    void GuiContext::PushClipRect( glm::vec2 min, glm::vec2 max ) {
        clipRectStack.push_back( IntersectGuiRect( GetClipRect(), GuiRect { min, max } ) );
    }

    void GuiContext::PopClipRect() {
        SL_ASSERT_MSG( !clipRectStack.empty(), "GuiContext::PopClipRect called without a matching PushClipRect" );
        clipRectStack.pop_back();
    }

    GuiRect GuiContext::GetClipRect() const {
        return clipRectStack.empty() ? UnboundedGuiRect() : clipRectStack.back();
    }

} // namespace sloth
