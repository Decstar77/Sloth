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
        ENTITY_TYPE_PLAYER,
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
        PlayerCapsule,
    };

    struct RigidBodySpawnData {
        bool                    createRigidBody = false;
        RigidBodyShape          shape = RigidBodyShape::Box;
        glm::vec3               halfExtents = { 0.5f, 0.5f, 0.5f };
        f32                     radius = 0.5f;
        f32                     height = 1.8f; // PlayerCapsule only: total capsule height (including hemisphere caps).
        BodyMotionType          motionType = BodyMotionType::Dynamic;
        f32                     friction = 0.5f;
        f32                     restitution = 0.0f;
    };

    struct PropData {
    };

    struct PlayerData {
        f32 eyeHeight = 0.7f;   // Offset above the capsule's center (its origin) the FPS camera sits at.
        f32 moveSpeed = 5.0f;   // Walk speed in m/s.
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
        RigidBody          rigidBody;      // ENTITY_TYPE_PLAYER never sets this - see character below.
        CharacterHandle    character;      // Only valid for ENTITY_TYPE_PLAYER, backed by Jolt's CharacterVirtual rather than a RigidBody.
        RigidBodySpawnData rigidBodyData;

        union {
            PropData    prop;
            PlayerData  player;
        };
    };

    Entity          MakeEntity( EntityType type, glm::vec3 position );
    const char *    ToString( EntityType type );

}
