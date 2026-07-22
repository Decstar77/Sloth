#pragma once

#include "core/sloth_defines.h"
#include "core/sloth_string.h"

namespace sloth
{
    // FNV-1a offset basis - the root seed GuiContext's ID stack starts from.
    using GuiId = u32;
    constexpr GuiId InvalidGuiId = 0;
    constexpr GuiId GuiIdRootSeed = 2166136261u;

    GuiId           HashGuiId( StringView label, GuiId seed = GuiIdRootSeed );
    GuiId           HashGuiId( i32 value, GuiId seed = GuiIdRootSeed );
    StringView      StripGuiLabelHash( StringView label );

} // namespace sloth
