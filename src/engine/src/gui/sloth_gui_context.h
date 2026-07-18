#pragma once

#include "core/sloth_defines.h"
#include "gui/sloth_gui_id.h"
#include "renderer/sloth_gui_rect.h"

#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

namespace sloth
{
    class Input;

    // Per-ID persistent scratch storage for widget state that doesn't fit
    // the three built-in hot/active/focused flags on GuiContext - a
    // scrollable panel's scroll offset, a slider's drag anchor, a
    // collapsible header's open/closed flag, and so on. A value defaults to
    // zero/false the first time a given GuiId is looked up (see
    // GetOrAddXxx()), so a widget never needs to explicitly initialize its
    // own entry - it just reads-or-creates on every call.
    class GuiStorage
    {
    public:
        bool& GetOrAddBool(GuiId id, bool defaultValue = false);
        i32& GetOrAddInt(GuiId id, i32 defaultValue = 0);
        f32& GetOrAddFloat(GuiId id, f32 defaultValue = 0.0f);
        glm::vec2& GetOrAddVec2(GuiId id, glm::vec2 defaultValue = glm::vec2(0.0f));

        void Clear();

    private:
        // One value slot per GuiId, reused across whichever of the four
        // accessors a widget happens to call for that ID - a widget is
        // expected to consistently use the same accessor for a given ID, the
        // union doesn't tag which type is "really" stored.
        union Value
        {
            bool AsBool;
            i32 AsInt;
            f32 AsFloat;
            glm::vec2 AsVec2;

            Value() : AsVec2(0.0f) {}
        };

        std::unordered_map<GuiId, Value> values;
    };

    // Owns everything an immediate-mode widget needs to persist across
    // frames despite having no object of its own: the ID stack that scopes
    // widget hashes (see sloth_gui_id.h), the three interaction flags every
    // widget derives its behavior/styling from (hot/active/focused), and a
    // GuiStorage for anything more widget-specific. This is plumbing only -
    // no widget functions (Button, Slider, ...) live here yet; those will be
    // built on top of PushId/GetId/SetHot/SetActive/GuiStorage.
    //
    // One GuiContext drives one GUI - own it explicitly (like GuiRenderer or
    // TextRenderer) and pass it to widget calls, rather than reaching for a
    // global/singleton. Nothing about Sloth's single-window model requires
    // more than one GuiContext to ever exist, but nothing here assumes that
    // either.
    class GuiContext
    {
    public:
        GuiContext() = default;

        SL_NON_COPYABLE(GuiContext);
        SL_NON_MOVABLE(GuiContext);

        // Call once per frame before any widget calls. Resets HotId - every
        // widget that wants it must re-claim it this frame via SetHot()
        // during its own hit-test, so hover state can never go stale if a
        // widget stops being drawn.
        void NewFrame(const Input& input);

        // Call once per frame after all widget calls; asserts PushId/PopId
        // balanced out.
        void EndFrame();

        // --- ID stack ----------------------------------------------------
        // PushId/PopId must balance within a frame. GetId() does not push -
        // it just computes what a leaf widget's ID would be under the
        // current scope, which is what most widget calls actually want
        // (PushId is for scoping a whole subtree, e.g. one per list row).
        void PushId(StringView label);
        void PushId(i32 value);
        void PopId();
        GuiId GetId(StringView label) const;
        GuiId GetId(i32 value) const;

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
        bool IsHot(GuiId id) const { return hotId == id; }
        bool IsActive(GuiId id) const { return activeId == id; }
        bool IsFocused(GuiId id) const { return focusedId == id; }
        bool IsAnyActive() const { return activeId != InvalidGuiId; }

        void SetHot(GuiId id);
        void SetActive(GuiId id);
        void ClearActive();
        void SetFocused(GuiId id);
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
        static bool IsPointInRect(glm::vec2 point, glm::vec2 min, glm::vec2 max);

        // --- Clip rect -----------------------------------------------------
        // Tracks the same nested clip region as GuiRenderer::PushClipRect/
        // PopClipRect (see that class for why the two stacks are separate
        // rather than shared), so hit-testing agrees with what's actually
        // visible: a widget scrolled out of view under a clipped panel
        // shouldn't be clickable just because the mouse still overlaps its
        // raw rect. Widgets should gate their own hover test with
        // IsPointVisible() in addition to IsPointInRect(). Must balance with
        // PopClipRect() before the following EndFrame().
        void PushClipRect(glm::vec2 min, glm::vec2 max);
        void PopClipRect();
        GuiRect GetClipRect() const;
        bool IsPointVisible(glm::vec2 point) const { return GetClipRect().Contains(point); }

        GuiStorage& GetStorage() { return storage; }

    private:
        std::vector<GuiId> idStack{ GuiIdRootSeed };
        std::vector<GuiRect> clipRectStack;

        GuiId hotId = InvalidGuiId;
        GuiId activeId = InvalidGuiId;
        GuiId focusedId = InvalidGuiId;

        glm::vec2 mousePos = glm::vec2(0.0f);
        bool mouseDown = false;
        bool mousePressed = false;
        bool mouseReleased = false;

        GuiStorage storage;
    };

} // namespace sloth
