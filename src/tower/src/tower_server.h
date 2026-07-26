#pragma once

#include <core/sloth_defines.h>
#include <physics/sloth_physics_world.h>
#include <network/sloth_network.h>

#include "tower_protocol.h"
#include "tower_world.h"

#include <vector>

namespace tower {

    // Shared between SandboxTowerServer and SandboxTower: the client uses
    // these to find/manage a locally-spawned dev server (see
    // SandboxTower's main.cpp).
    constexpr u16          DefaultServerPort = 27020;
    constexpr const char * ServerMutexName = "Global\\SlothTowerServerMutex";

    // Headless, authoritative driver for a Tower dedicated server process:
    // owns the world simulation and the network listener, but no rendering,
    // camera, or window - unlike TowerGame, which owns GL resources and
    // can't run without a context. Meant to run inside a process that called
    // sloth::Engine::InitHeadless().
    class TowerServer {
      public:
        void                    Init( u16 port );
        void                    Shutdown();

        void                    Update( f32 deltaTime );

        TowerWorld &            GetWorld() { return world; }
        sloth::PhysicsWorld &   GetPhysicsWorld() { return physicsWorld; }

      private:
        void                    HandleNetworkEvents();
        void                    BroadcastWorldSnapshot();
        void                    HandlePlayerShot( sloth::NetConnection shooterConnection, const PlayerShotMessage & message );

        // Player state is tracked here, keyed by connection, rather than as
        // TowerWorld entities: movement is client-authoritative for now (the
        // client runs its own CharacterVirtual and just reports where it
        // ended up), so the server has no physics representation of players
        // to simulate, only the latest reported transform to relay to peers.
        // Health lives here for the same reason - it's server-owned state
        // about a connection, not a simulated entity.
        struct ServerPlayer {
            sloth::NetConnection connection;
            u32                  id = 0; // == connection.Id
            glm::vec3            position { 0.0f, 0.0f, 0.0f };
            glm::quat            rotation { 1.0f, 0.0f, 0.0f, 0.0f };
            bool                 hasState = false; // Have we received at least one PlayerState from them yet?
            f32                  health = 100.0f;
        };

      private:
        TowerWorld                  world;
        sloth::PhysicsWorld         physicsWorld;
        sloth::NetworkSystem        network;
        std::vector<ServerPlayer>   players;
    };

} // namespace tower
