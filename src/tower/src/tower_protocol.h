#pragma once

#include <core/sloth_defines.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace tower {

    // Minimal client/server wire protocol, shared by TowerServer and
    // TowerGame. Messages are plain POD structs sent as raw bytes via
    // NetworkSystem::SendMessage and read back through NetEvent::data - no
    // versioning or endianness handling, since both sides are always built
    // from the same source. Revisit if that stops being true.
    enum class MessageType : u8 {
        Welcome = 1,        // server -> client, once, right after Connected: tells the client its assigned playerId.
        PlayerState = 2,    // client -> server, every frame: the sender's own position/rotation.
        WorldSnapshot = 3,  // server -> client, every tick: every known player's position/rotation.
        PlayerShot = 4,     // client -> server, on fire: a hitscan shot to resolve.
        PlayerHealth = 5,   // server -> client (target only): your health changed.
        PlayerRespawn = 6,  // server -> client (target only): you died, teleport to this position.
    };

    struct WelcomeMessage {
        MessageType type = MessageType::Welcome;
        u32         playerId = 0;
    };

    struct PlayerStateMessage {
        MessageType type = MessageType::PlayerState;
        glm::vec3   position { 0.0f, 0.0f, 0.0f };
        glm::quat   rotation { 1.0f, 0.0f, 0.0f, 0.0f };
    };

    struct PlayerSnapshotEntry {
        u32         playerId = 0;
        glm::vec3   position { 0.0f, 0.0f, 0.0f };
        glm::quat   rotation { 1.0f, 0.0f, 0.0f, 0.0f };
    };

    // Not fixed-size - in the actual message buffer, this header is
    // immediately followed by `playerCount` PlayerSnapshotEntry structs, so
    // it's built/parsed by hand rather than cast wholesale like the fixed
    // messages above.
    struct WorldSnapshotHeader {
        MessageType type = MessageType::WorldSnapshot;
        u32         playerCount = 0;
    };

    // Hitscan: the server resolves the hit against its own tracked player
    // positions (see TowerServer::HandlePlayerShot), the client only
    // reports the ray it fired.
    struct PlayerShotMessage {
        MessageType type = MessageType::PlayerShot;
        glm::vec3   origin { 0.0f, 0.0f, 0.0f };
        glm::vec3   direction { 0.0f, 0.0f, 1.0f };
    };

    struct PlayerHealthMessage {
        MessageType type = MessageType::PlayerHealth;
        f32         health = 100.0f;
    };

    struct PlayerRespawnMessage {
        MessageType type = MessageType::PlayerRespawn;
        glm::vec3   position { 0.0f, 0.0f, 0.0f };
    };

} // namespace tower
