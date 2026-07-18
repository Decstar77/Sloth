#pragma once

#include "dust_entity.h"

#include <vector>

namespace dust {

    class DustWorld {
    public:

        std::vector<Entity>   entities;
        std::vector<EntityId> simHot;
        std::vector<EntityId> simWarm;
        std::vector<EntityId> simCold;
    };

}


