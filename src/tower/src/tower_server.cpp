#include "tower_server.h"

#include "tower_protocol.h"

#include <core/sloth_engine.h>

#include <cstring>

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
        BroadcastWorldSnapshot();
    }

    void TowerServer::HandleNetworkEvents() {
        for ( usize i = 0; i < network.GetEventCount(); i++ ) {
            const NetEvent & event = network.GetEvents()[i];

            switch ( event.type ) {
                case NetEventType::IncomingConnection:
                    SL_LOG_INFO( "TowerServer: incoming connection %u, accepting", event.connection.Id );
                    network.AcceptConnection( event.connection );
                    break;

                case NetEventType::Connected: {
                    SL_LOG_INFO( "TowerServer: connection %u established", event.connection.Id );

                    ServerPlayer player;
                    player.connection = event.connection;
                    player.id = event.connection.Id;
                    players.push_back( player );

                    WelcomeMessage welcome;
                    welcome.playerId = player.id;
                    network.SendMessage( event.connection, &welcome, sizeof( welcome ), NetSendType::Reliable );
                } break;

                case NetEventType::Disconnected: {
                    SL_LOG_INFO( "TowerServer: connection %u disconnected", event.connection.Id );

                    for ( auto it = players.begin(); it != players.end(); ++it ) {
                        if ( it->connection.Id == event.connection.Id ) {
                            players.erase( it );
                            break;
                        }
                    }
                } break;

                case NetEventType::MessageReceived: {
                    if ( event.dataSize < sizeof( MessageType ) ) {
                        break;
                    }

                    MessageType type;
                    memcpy( &type, event.data, sizeof( MessageType ) );

                    if ( type == MessageType::PlayerState && event.dataSize == sizeof( PlayerStateMessage ) ) {
                        PlayerStateMessage message;
                        memcpy( &message, event.data, sizeof( message ) );

                        for ( ServerPlayer & player : players ) {
                            if ( player.connection.Id == event.connection.Id ) {
                                player.position = message.position;
                                player.rotation = message.rotation;
                                player.hasState = true;
                                break;
                            }
                        }
                    }
                } break;
            }
        }
    }

    void TowerServer::BroadcastWorldSnapshot() {
        if ( players.empty() ) {
            return;
        }

        // Only players who've reported at least one state are included, so a
        // freshly connected player doesn't briefly appear at the origin for
        // everyone else before their first PlayerState arrives.
        std::vector<PlayerSnapshotEntry> entries;
        entries.reserve( players.size() );
        for ( const ServerPlayer & player : players ) {
            if ( player.hasState ) {
                entries.push_back( { player.id, player.position, player.rotation } );
            }
        }

        usize bufferSize = sizeof( WorldSnapshotHeader ) + entries.size() * sizeof( PlayerSnapshotEntry );
        std::vector<u8> buffer( bufferSize );

        WorldSnapshotHeader header;
        header.playerCount = static_cast<u32>( entries.size() );
        memcpy( buffer.data(), &header, sizeof( header ) );
        if ( !entries.empty() ) {
            memcpy( buffer.data() + sizeof( header ), entries.data(), entries.size() * sizeof( PlayerSnapshotEntry ) );
        }

        for ( const ServerPlayer & player : players ) {
            network.SendMessage( player.connection, buffer.data(), buffer.size(), NetSendType::Unreliable );
        }
    }

} // namespace tower
