#include "tower_server.h"

#include "tower_protocol.h"

#include <core/sloth_engine.h>

#include <cstring>
#include <random>

using namespace sloth;

namespace tower {

    static constexpr f32 MaxHealth = 100.0f;
    static constexpr f32 ShotDamage = 25.0f;
    static constexpr f32 ShotMaxRange = 200.0f;
    static constexpr f32 PlayerHitHeight = 1.8f;   // Matches MakeEntity's ENTITY_TYPE_PLAYER capsule height (tower_entity.cpp).
    static constexpr f32 PlayerHitRadius = 0.5f;   // Padded past the visual capsule's 0.35 radius, since the hands box (see TowerGame::RenderPlayerHands) sticks out further than that.

    static f32 RandomRange( f32 min, f32 max ) {
        static std::mt19937 rng { std::random_device {}() };
        std::uniform_real_distribution<f32> dist( min, max );
        return dist( rng );
    }

    static glm::vec3 RandomSpawnPosition() {
        // Keeps clear of the floor's edges (30x30 half-extents - see Init()
        // below) and matches the initial spawn's drop height.
        return glm::vec3( RandomRange( -25.0f, 25.0f ), 3.0f, RandomRange( -25.0f, 25.0f ) );
    }

    static bool RaySphereIntersect( const glm::vec3 & origin, const glm::vec3 & direction, const glm::vec3 & center, f32 radius, f32 & outT ) {
        glm::vec3 oc = origin - center;
        f32 b = glm::dot( oc, direction );
        f32 c = glm::dot( oc, oc ) - radius * radius;
        f32 discriminant = b * b - c;
        if ( discriminant < 0.0f ) {
            return false;
        }

        f32 sqrtDiscriminant = glm::sqrt( discriminant );
        f32 t = -b - sqrtDiscriminant;
        if ( t < 0.0f ) {
            t = -b + sqrtDiscriminant; 
        }

        if ( t < 0.0f || t > outT ) {
            return false;
        }

        outT = t;
        return true;
    }

    // Approximates a ray-vs-vertical-capsule test as several spheres
    // stacked from feet to head, rather than exact capsule math - good
    // enough for this prototype's placeholder geometry, and it reuses
    // RaySphereIntersect's "outT is the current best" shrinking behavior to
    // naturally pick the nearest sample along the body.
    static bool RayPlayerIntersect( const glm::vec3 & origin, const glm::vec3 & direction, const glm::vec3 & feet, f32 & outT ) {
        constexpr i32 sampleCount = 6;
        bool hitAny = false;
        for ( i32 i = 0; i < sampleCount; i++ ) {
            f32 height = PlayerHitHeight * static_cast<f32>( i ) / static_cast<f32>( sampleCount - 1 );
            glm::vec3 center = feet + glm::vec3( 0.0f, height, 0.0f );
            if ( RaySphereIntersect( origin, direction, center, PlayerHitRadius, outT ) ) {
                hitAny = true;
            }
        }
        return hitAny;
    }

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
                    } else if ( type == MessageType::PlayerShot && event.dataSize == sizeof( PlayerShotMessage ) ) {
                        PlayerShotMessage message;
                        memcpy( &message, event.data, sizeof( message ) );
                        HandlePlayerShot( event.connection, message );
                    }
                } break;
            }
        }
    }

    void TowerServer::HandlePlayerShot( NetConnection shooterConnection, const PlayerShotMessage & message ) {
        f32 directionLength = glm::length( message.direction );
        if ( directionLength < 0.0001f ) {
            return; // malformed - a real client always fires along its camera forward, which is unit-length
        }
        glm::vec3 direction = message.direction / directionLength;

        // Gunshot sound cue for everyone but the shooter - they already
        // played it locally the instant they fired, for zero-latency
        // feedback (see TowerGame::UpdateShooting). Sent unconditionally,
        // hit or miss - a gunshot makes noise either way.
        PlayerShotFiredMessage shotFired;
        shotFired.origin = message.origin;
        for ( const ServerPlayer & listener : players ) {
            if ( listener.connection.Id != shooterConnection.Id ) {
                network.SendMessage( listener.connection, &shotFired, sizeof( shotFired ), NetSendType::Unreliable );
            }
        }

        // World geometry (props, floor) blocks the shot.
        f32 closestT = ShotMaxRange;
        RayCastHit worldHit;
        if ( physicsWorld.Raycast( message.origin, direction, ShotMaxRange, worldHit ) ) {
            closestT = glm::length( worldHit.point - message.origin );
        }

        // Nearest-hit-wins across everyone but the shooter
        ServerPlayer * target = nullptr;
        for ( ServerPlayer & candidate : players ) {
            if ( candidate.connection.Id == shooterConnection.Id || !candidate.hasState ) {
                continue;
            }

            if ( RayPlayerIntersect( message.origin, direction, candidate.position, closestT ) ) {
                target = &candidate;
            }
        }

        if ( target == nullptr ) {
            return;
        }

        target->health -= ShotDamage;
        SL_LOG_INFO( "TowerServer: player %u hit player %u (health now %.0f)", shooterConnection.Id, target->id, static_cast<f64>( target->health ) );

        // Hit sound cue, positional at the target, for everyone (shooter,
        // target, and bystanders alike) - unlike health/respawn below,
        // which only the target needs to know about.
        PlayerHitConfirmMessage hitConfirm;
        hitConfirm.position = target->position + glm::vec3( 0.0f, PlayerHitHeight * 0.5f, 0.0f );
        for ( const ServerPlayer & listener : players ) {
            network.SendMessage( listener.connection, &hitConfirm, sizeof( hitConfirm ), NetSendType::Unreliable );
        }

        if ( target->health <= 0.0f ) {
            target->health = MaxHealth;
            target->position = RandomSpawnPosition();

            PlayerRespawnMessage respawn;
            respawn.position = target->position;
            network.SendMessage( target->connection, &respawn, sizeof( respawn ), NetSendType::Reliable );

            SL_LOG_INFO( "TowerServer: player %u died, respawning", target->id );
        }

        PlayerHealthMessage healthMessage;
        healthMessage.health = target->health;
        network.SendMessage( target->connection, &healthMessage, sizeof( healthMessage ), NetSendType::Reliable );
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
