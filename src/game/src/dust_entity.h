#pragma once

#include <core/sloth_defines.h>
#include <physics/sloth_physics_world.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace sloth;

namespace dust {

    enum EntityType {
        ENTITY_TYPE_INVALID = 0,
        ENTITY_TYPE_PROP, // A rigid-body-backed object with no gameplay logic of its own (crates, rocks, debris, ...).
    };
     
    struct EntityId {
        i32 index      = -1;
        i32 generation = 0;
    };

    constexpr EntityId INVALID_ENTITY_ID = {};

    inline bool operator==(const EntityId& a, const EntityId& b) { return a.index == b.index && a.generation == b.generation; }
    inline bool operator!=(const EntityId& a, const EntityId& b) { return !(a == b); }

    enum class PropShape {
        Box,
        Sphere,
    };

    struct PropData {
        RigidBody               physicsBody;
        PropShape               propShape = PropShape::Box;
        glm::vec3               halfExtents = { 0.5f, 0.5f, 0.5f };
        f32                     radius = 0.5f;
        BodyMotionType          motionType = BodyMotionType::Dynamic;
        f32                     friction = 0.5f;
        f32                     restitution = 0.0f;
    };

    struct Entity {
        glm::vec3   position;
        glm::quat   rotation;
        f32         scale;

        EntityType  type;
        EntityId    id;

        union {
            PropData prop;
        };
    };

    Entity MakeEntity(EntityType type, glm::vec3 position);
}
