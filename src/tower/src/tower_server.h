#pragma once

#include <core/sloth_defines.h>
#include <physics/sloth_physics_world.h>
#include <network/sloth_network.h>

#include "tower_world.h"

namespace tower {

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

      private:
        TowerWorld              world;
        sloth::PhysicsWorld     physicsWorld;
        sloth::NetworkSystem    network;
    };

} // namespace tower
