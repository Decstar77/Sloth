#pragma once

#include "core/sloth_defines.h"
#include "gui/sloth_gui_id.h"
#include "renderer/sloth_gui_rect.h"

#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

namespace sloth {
    class Input;

    class GuiStorage {
      public:
        bool &      GetOrAddBool( GuiId id, bool defaultValue = false );
        i32 &       GetOrAddInt( GuiId id, i32 defaultValue = 0 );
        f32 &       GetOrAddFloat( GuiId id, f32 defaultValue = 0.0f );
        glm::vec2 & GetOrAddVec2( GuiId id, glm::vec2 defaultValue = glm::vec2( 0.0f ) );

        void Clear();

      private:
        union Value {
            bool AsBool;
            i32 AsInt;
            f32 AsFloat;
            glm::vec2 AsVec2;

            Value() : AsVec2( 0.0f ) {}
        };

        std::unordered_map<GuiId, Value> values;
    };

    class GuiContext {
      public:
        GuiContext() = default;

        SL_NON_COPYABLE( GuiContext );
        SL_NON_MOVABLE( GuiContext );

        void NewFrame( const Input & input );
        void EndFrame();

        // --- ID stack ----------------------------------------------------
        // PushId/PopId must balance within a frame. GetId() does not push -
        // it just computes what a leaf widget's ID would be under the
        // current scope, which is what most widget calls actually want
        // (PushId is for scoping a whole subtree, e.g. one per list row).
        void PushId( StringView label );
        void PushId( i32 value );
        void PopId();
        GuiId GetId( StringView label ) const;
        GuiId GetId( i32 value ) const;

        // --- Hot / active / focused ---------------------------------------
        // "Hot"     = mouse is over it this frame. Last SetHot() call in the
        //             frame wins, so widgets submitted later (i.e. drawn on
        //             top, like popups/tooltips) naturally take priority
        //             over ones underneath without any explicit z-ordering.
        // "Active"  = currently being interacted with (e.g. mouse pressed
        //             down on it and not yet released). Exclusive - claiming
        //             it locks out every other widget's SetHot() until
        //             ClearActive(), so a drag can't lose mouse priority to
        //             whatever the pointer happens to cross.
        // "Focused" = has keyboard focus (text input, tab order). Persists
        //             across frames until something else claims it or
        //             ClearFocused() is called explicitly - there's no
        //             automatic click-away-clears-focus yet, that belongs
        //             with the widget-submission loop once one exists.
        bool IsHot( GuiId id ) const { return hotId == id; }
        bool IsActive( GuiId id ) const { return activeId == id; }
        bool IsFocused( GuiId id ) const { return focusedId == id; }
        bool IsAnyActive() const { return activeId != InvalidGuiId; }

        void SetHot( GuiId id );
        void SetActive( GuiId id );
        void ClearActive();
        void SetFocused( GuiId id );
        void ClearFocused();

        // True once anything has claimed hot or active this frame - a cheap
        // stand-in for Dear ImGui's io.WantCaptureMouse, for gameplay code
        // (e.g. DustCamera) deciding whether to also react to the same
        // click/hover.
        bool WantsMouseInput() const { return hotId != InvalidGuiId || activeId != InvalidGuiId; }

        // --- Mouse ---------------------------------------------------------
        glm::vec2 GetMousePos() const { return mousePos; }
        bool IsMouseDown() const { return mouseDown; }
        bool IsMousePressed() const { return mousePressed; }
        bool IsMouseReleased() const { return mouseReleased; }
        static bool IsPointInRect( glm::vec2 point, glm::vec2 min, glm::vec2 max );

        // --- Clip rect -----------------------------------------------------
        // Tracks the same nested clip region as GuiRenderer::PushClipRect/
        // PopClipRect (see that class for why the two stacks are separate
        // rather than shared), so hit-testing agrees with what's actually
        // visible: a widget scrolled out of view under a clipped panel
        // shouldn't be clickable just because the mouse still overlaps its
        // raw rect. Widgets should gate their own hover test with
        // IsPointVisible() in addition to IsPointInRect(). Must balance with
        // PopClipRect() before the following EndFrame().
        void PushClipRect( glm::vec2 min, glm::vec2 max );
        void PopClipRect();
        GuiRect GetClipRect() const;
        bool IsPointVisible( glm::vec2 point ) const { return GetClipRect().Contains( point ); }

        GuiStorage & GetStorage() { return storage; }

      private:
        std::vector<GuiId> idStack { GuiIdRootSeed };
        std::vector<GuiRect> clipRectStack;

        GuiId hotId = InvalidGuiId;
        GuiId activeId = InvalidGuiId;
        GuiId focusedId = InvalidGuiId;

        glm::vec2 mousePos = glm::vec2( 0.0f );
        bool mouseDown = false;
        bool mousePressed = false;
        bool mouseReleased = false;

        GuiStorage storage;
    };

} // namespace sloth
