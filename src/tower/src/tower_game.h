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
#include <unordered_map>
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
        void                    ApplyWorldSnapshot( const u8 * data, usize size );

    private:
        TowerWorld                          world;
        TowerCamera                         camera;
        EntityId                            playerId;

        // Rendering stuff, need a better place to put all this
        std::unique_ptr<sloth::Shader>      shader;
        std::unique_ptr<sloth::StaticMesh>  floorMesh;
        std::unique_ptr<sloth::StaticMesh>  boxMesh;
        std::unique_ptr<sloth::StaticMesh>  capsuleMesh; // No capsule primitive in Geometry yet - a cylinder stands in for other players' capsules.

        sloth::PhysicsWorld                 physicsWorld;

        sloth::NetworkSystem                network;
        sloth::NetConnection                serverConnection;

        // See tower_protocol.h. Movement is client-authoritative: each
        // client reports its own transform every frame and mirrors everyone
        // else's via ENTITY_TYPE_REMOTE_PLAYER entities driven straight from
        // the server's snapshots (no physics/prediction on remote players).
        u32                                  localPlayerId = 0;
        bool                                 hasLocalPlayerId = false;
        std::unordered_map<u32, EntityId>    remotePlayers;
    };

}
