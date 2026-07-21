#pragma once

#include <core/sloth_defines.h>
#include <vector>

namespace dust {

    struct EntityId;

    enum FactionType {
        FACTION_TYPE_NEUTRAL = 0,
        FACTION_TYPE_REMNANT,
        FACTION_TYPE_RUSTBORN,
        FACTION_TYPE_ZENITH
    };

    struct Faction {
        FactionType             type;
        i64                     credits;
        std::vector<EntityId>   entities;
    };

}
