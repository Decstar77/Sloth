#pragma once

#include <core/sloth_defines.h>
#include <core/sloth_string.h>
#include <network/sloth_network.h>
#include <physics/sloth_physics_world.h>
#include <renderer/sloth_shader.h>
#include <renderer/sloth_static_mesh.h>

#include "tower_camera.h"
#include "tower_world.h"

#include <memory>
#include <vector>

namespace tower {

    class TowerGame {
    public:
        void                    Init();
        void                    Shutdown();

        void                    Update( f32 deltaTime );
        void                    Render();

        TowerCamera &           GetCamera() { return camera; }
        sloth::PhysicsWorld &   GetPhysicsWorld() { return physicsWorld; }
        TowerWorld &            GetWorld() { return world; }

        sloth::NetConnectionState GetServerConnectionState() const { return network.GetConnectionState( serverConnection ); }

    private:
        void                    UpdatePlayerMovement( f32 deltaTime );
        void                    UpdateNetworking();

    private:
        TowerWorld                          world;
        TowerCamera                         camera;
        EntityId                            playerId;

        // Rendering stuff, need a better place to put all this
        std::unique_ptr<sloth::Shader>      shader;
        std::unique_ptr<sloth::StaticMesh>  floorMesh;
        std::unique_ptr<sloth::StaticMesh>  boxMesh;

        sloth::PhysicsWorld                 physicsWorld;

        // No wire protocol yet (see TowerServer::HandleNetworkEvents) - for
        // now this just establishes and tracks the connection to the local
        // dev server SandboxTower spawns on startup.
        sloth::NetworkSystem                network;
        sloth::NetConnection                serverConnection;
    };

}
