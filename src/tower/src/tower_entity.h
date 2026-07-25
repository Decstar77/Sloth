#pragma once

#include <core/sloth_defines.h>

#include <physics/sloth_physics_world.h>
#include <renderer/sloth_render_model.h>

using namespace sloth;

namespace tower {

    struct Entity;
    enum EntityType {
        ENTITY_TYPE_INVALID = 0,
        ENTITY_TYPE_PROP,
    };

    struct EntityId {
        i32 index      = -1;
        i32 generation = 0;
    };

    constexpr EntityId INVALID_ENTITY_ID = {};

    inline bool operator==( const EntityId & a, const EntityId & b ) { return a.index == b.index && a.generation == b.generation; }
    inline bool operator!=( const EntityId & a, const EntityId & b ) { return !( a == b ); }

    enum class RigidBodyShape {
        Box,
        Sphere,
    };

    struct RigidBodySpawnData {
        bool                    createRigidBody = false;
        RigidBodyShape          shape = RigidBodyShape::Box;
        glm::vec3               halfExtents = { 0.5f, 0.5f, 0.5f };
        f32                     radius = 0.5f;
        BodyMotionType          motionType = BodyMotionType::Dynamic;
        f32                     friction = 0.5f;
        f32                     restitution = 0.0f;
    };

    struct PropData {
    };

    struct Entity {
        // Entity
        EntityType  type;
        EntityId    id;

        // Transform
        glm::vec3   position;
        glm::quat   rotation;
        f32         scale;

        // Rendering
        RenderModel        renderModel;

        // Physics
        RigidBody          rigidBody;
        RigidBodySpawnData rigidBodyData;

        union {
            PropData    prop;
        };
    };

    Entity          MakeEntity( EntityType type, glm::vec3 position );
    const char *    ToString( EntityType type );

}
