#pragma once

#include <core/sloth_defines.h>

#include <glm/glm.hpp>

namespace dust {

    enum EntityType {
        ENTITY_TYPE_INVALID = 0,
    };

    struct EntityId {
        i32 index;
        i32 generation;
    };

    constexpr EntityId INVALID_ENTITY_ID = {};

    struct Entity {
        glm::vec3   position;
        glm::mat3   orientation;
        f32         scale;

        EntityType  type;
        EntityId    id;

        union {
            // Stuff in here
        };
    };




}


