#include "tower_server.h"

#include <core/sloth_engine.h>

using namespace sloth;

namespace tower {

    void TowerServer::Init( u16 port ) {
        world.Init( &physicsWorld );

        // Same starting layout as TowerGame::Init(), minus everything
        // rendering-related - the server only needs entity/physics state,
        // it never draws anything, so renderModel is left unset.
        {
            glm::vec3 halfExtents( 30.0f, 0.5f, 30.0f );

            Entity entity = MakeEntity( ENTITY_TYPE_PROP, { 0.0f, -0.5f, 0.0f } );
            entity.rigidBodyData.shape = RigidBodyShape::Box;
            entity.rigidBodyData.halfExtents = halfExtents;
            entity.rigidBodyData.motionType = BodyMotionType::Static;
            world.SpawnEntity( entity );
        }

        {
            glm::vec3 halfExtents( 0.5f, 0.5f, 0.5f );

            for ( i32 i = 0; i < 6; ++i ) {
                Entity entity = MakeEntity( ENTITY_TYPE_PROP, { 0.0f, 1.0f + static_cast<f32>( i ) * 1.1f, 0.0f } );
                entity.rigidBodyData.shape = RigidBodyShape::Box;
                entity.rigidBodyData.halfExtents = halfExtents;
                entity.rigidBodyData.motionType = BodyMotionType::Dynamic;
                world.SpawnEntity( entity );
            }
        }

        world.FlushPendingChanges();

        bool listening = network.Listen( port );
        SL_ASSERT_MSG( listening, "TowerServer: failed to listen on port %u", (u32)port );
        SL_UNUSED( listening );

        SL_LOG_INFO( "TowerServer: listening on port %u", (u32)port );
    }

    void TowerServer::Shutdown() {
    }

    void TowerServer::Update( f32 deltaTime ) {
        physicsWorld.Update( deltaTime );
        world.SyncPhysicsTransforms();
        world.FlushPendingChanges();

        network.Update( Engine::Get().GetFrameArena() );
        HandleNetworkEvents();
    }

    void TowerServer::HandleNetworkEvents() {
        for ( usize i = 0; i < network.GetEventCount(); i++ ) {
            const NetEvent & event = network.GetEvents()[i];

            switch ( event.type ) {
                case NetEventType::IncomingConnection:
                    SL_LOG_INFO( "TowerServer: incoming connection %u, accepting", event.connection.Id );
                    network.AcceptConnection( event.connection );
                    break;

                case NetEventType::Connected:
                    SL_LOG_INFO( "TowerServer: connection %u established", event.connection.Id );
                    break;

                case NetEventType::Disconnected:
                    SL_LOG_INFO( "TowerServer: connection %u disconnected", event.connection.Id );
                    break;

                case NetEventType::MessageReceived:
                    // No wire protocol defined yet - just observe traffic for now.
                    SL_LOG_INFO( "TowerServer: message from %u (%zu bytes)", event.connection.Id, event.dataSize );
                    break;
            }
        }
    }

} // namespace tower
