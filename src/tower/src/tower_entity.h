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
        ENTITY_TYPE_REMOTE_PLAYER, // A visual-only stand-in for another client's player, driven by network snapshots rather than local input/physics.
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
        f32  eyeHeight = 1.6f;         // Eye offset above the feet (the capsule's origin) while standing.
        f32  crouchEyeHeight = 0.9f;   // Eye offset above the feet while crouched.
        f32  moveSpeed = 5.0f;         // Walk speed in m/s.
        f32  sprintMoveSpeed = 8.5f;   // Walk speed in m/s while sprinting.
        f32  crouchMoveSpeed = 2.5f;   // Walk speed in m/s while crouched (sprint is disabled while crouched).
        f32  jumpSpeed = 5.0f;         // Vertical velocity applied on jump, in m/s.
        f32  crouchHeight = 1.0f;      // Capsule height while crouched (standing height lives in rigidBodyData.height).
        bool isCrouching = false;      // Current stance; toggled by UpdatePlayerMovement, not user-set.
    };

    struct RemotePlayerData {
        u32 networkId = 0; // Server-assigned player id (see tower_protocol.h) this entity mirrors.
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
            PropData          prop;
            PlayerData        player;
            RemotePlayerData  remotePlayer;
        };
    };

    Entity          MakeEntity( EntityType type, glm::vec3 position );
    const char *    ToString( EntityType type );

}
