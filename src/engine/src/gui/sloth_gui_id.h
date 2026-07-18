#pragma once

#include "core/sloth_defines.h"
#include "core/sloth_string.h"

namespace sloth
{
    // Stable identifier for a widget, used to look up its persistent state
    // (hot/active/focused, and anything in GuiStorage) across frames.
    // Immediate-mode widget calls have no object to persist state on
    // directly, so every call re-derives the same GuiId from its label and
    // the current ID stack scope (GuiContext::PushId/PopId) and looks its
    // state up by that hash instead of storing it locally.
    using GuiId = u32;
    constexpr GuiId InvalidGuiId = 0;

    // FNV-1a offset basis - the root seed GuiContext's ID stack starts from.
    constexpr GuiId GuiIdRootSeed = 2166136261u;

    // FNV-1a over `label`'s bytes, continuing from `seed`. Chaining seeds is
    // what makes ID scoping work: PushId("Inventory") followed by a widget
    // labeled "Slot" hashes to a different ID than the same "Slot" label
    // used outside that scope, or under a different PushId(). Two widgets in
    // the same scope with the same visible label still collide unless
    // disambiguated - see StripGuiLabelHash() below for the "##" convention
    // that solves this without changing what's drawn.
    GuiId HashGuiId(StringView label, GuiId seed = GuiIdRootSeed);

    // Same idea for loop-generated widgets (e.g. one row per inventory slot)
    // where there's no natural label to hash - hashes the integer's bytes
    // instead.
    GuiId HashGuiId(i32 value, GuiId seed = GuiIdRootSeed);

    // Everything from the first "##" onward exists only to make the ID
    // unique and should never be shown to the user - e.g. "Save##File" and
    // "Save##Edit" both display as "Save" but hash to different IDs. Widgets
    // that draw a label should draw StripGuiLabelHash(label), not the label
    // itself; HashGuiId() should still be called on the full, unstripped
    // label so the "##" suffix keeps disambiguating.
    StringView StripGuiLabelHash(StringView label);

} // namespace sloth
