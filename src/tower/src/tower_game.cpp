#include "tower_game.h"

#include <core/sloth_engine.h>
#include <renderer/sloth_geometry.h>

#include "tower_protocol.h"
#include "tower_server.h" // for DefaultServerPort - see tower_server.h

#include <glad/gl.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>

using namespace sloth;

namespace tower {

    static const char * VertexShaderSource = R"(
        #version 450 core
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec3 aColor;

        uniform mat4 uViewProjection;
        uniform mat4 uModel;

        out vec3 vColor;

        void main()
        {
            vColor = aColor;
            gl_Position = uViewProjection * uModel * vec4(aPosition, 1.0);
        }
    )";

    static const char * FragmentShaderSource = R"(
        #version 450 core
        in vec3 vColor;
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(vColor, 1.0);
        }
    )";

    static std::unique_ptr<StaticMesh> UploadMesh( const MeshData & data ) {
        return std::make_unique<StaticMesh>( data.vertices.data(), static_cast<u32>( data.vertices.size() ), data.indices.data(), static_cast<u32>( data.indices.size() ) );
    }

    // Absolute paths, matching the font load below - keep consistent if that
    // ever moves back to a relative/working-directory-relative scheme.
    static const char * FootstepSounds[] = {
        "C:/Projects/2026/Sloth/assets/sounds/footsteps/Light-Armor-Concrete-Walking-1.wav",
        "C:/Projects/2026/Sloth/assets/sounds/footsteps/Light-Armor-Concrete-Walking-2.wav",
        "C:/Projects/2026/Sloth/assets/sounds/footsteps/Light-Armor-Concrete-Walking-3.wav",
        "C:/Projects/2026/Sloth/assets/sounds/footsteps/Light-Armor-Concrete-Walking-4.wav",
        "C:/Projects/2026/Sloth/assets/sounds/footsteps/Light-Armor-Concrete-Walking-5.wav",
        "C:/Projects/2026/Sloth/assets/sounds/footsteps/Light-Armor-Concrete-Walking-6.wav",
        "C:/Projects/2026/Sloth/assets/sounds/footsteps/Light-Armor-Concrete-Walking-7.wav",
        "C:/Projects/2026/Sloth/assets/sounds/footsteps/Light-Armor-Concrete-Walking-8.wav",
        "C:/Projects/2026/Sloth/assets/sounds/footsteps/Light-Armor-Concrete-Walking-9.wav",
        "C:/Projects/2026/Sloth/assets/sounds/footsteps/Light-Armor-Concrete-Walking-10.wav",
    };

    static const char * GunshotSounds[] = {
        "C:/Projects/2026/Sloth/assets/sounds/glock/gun_pistol_shot_01.wav",
        "C:/Projects/2026/Sloth/assets/sounds/glock/gun_pistol_shot_02.wav",
        "C:/Projects/2026/Sloth/assets/sounds/glock/gun_pistol_shot_03.wav",
        "C:/Projects/2026/Sloth/assets/sounds/glock/gun_pistol_shot_04.wav",
        "C:/Projects/2026/Sloth/assets/sounds/glock/gun_pistol_shot_05.wav",
    };

    static const char * HitSounds[] = {
        "C:/Projects/2026/Sloth/assets/sounds/bullet-hits/basic_hit_01.wav",
        "C:/Projects/2026/Sloth/assets/sounds/bullet-hits/basic_hit_02.wav",
        "C:/Projects/2026/Sloth/assets/sounds/bullet-hits/basic_hit_03.wav",
        "C:/Projects/2026/Sloth/assets/sounds/bullet-hits/basic_hit_04.wav",
        "C:/Projects/2026/Sloth/assets/sounds/bullet-hits/basic_hit_05.wav",
    };

    static const char * PickRandomSound( const char * const * paths, usize count ) {
        return paths[static_cast<usize>( rand() ) % count];
    }

    void TowerGame::Init() {
        glEnable( GL_DEPTH_TEST );

        world.Init( &physicsWorld );

        shader = std::make_unique<Shader>( VertexShaderSource, FragmentShaderSource );

        // Static floor.
        {
            glm::vec3 halfExtents( 30.0f, 0.5f, 30.0f );
            floorMesh = UploadMesh( Geometry::CreateBox( halfExtents.x * 2.0f, halfExtents.y * 2.0f, halfExtents.z * 2.0f, { 0.5f, 0.5f, 0.55f } ) );

            Entity entity = MakeEntity( ENTITY_TYPE_PROP, { 0.0f, -0.5f, 0.0f } );
            entity.renderModel = { shader.get(), floorMesh.get() };
            entity.rigidBodyData.shape = RigidBodyShape::Box;
            entity.rigidBodyData.halfExtents = halfExtents;
            entity.rigidBodyData.motionType = BodyMotionType::Static;
            world.SpawnEntity( entity );
        }

        // A few stacked boxes, standing in for future tower-blocks.
        {
            glm::vec3 halfExtents( 0.5f, 0.5f, 0.5f );
            boxMesh = UploadMesh( Geometry::CreateBox( halfExtents.x * 2.0f, halfExtents.y * 2.0f, halfExtents.z * 2.0f, { 0.85f, 0.55f, 0.25f } ) );

            for ( i32 i = 0; i < 6; ++i ) {
                Entity entity = MakeEntity( ENTITY_TYPE_PROP, { 0.0f, 1.0f + static_cast<f32>( i ) * 1.1f, 0.0f } );
                entity.renderModel = { shader.get(), boxMesh.get() };
                entity.rigidBodyData.shape = RigidBodyShape::Box;
                entity.rigidBodyData.halfExtents = halfExtents;
                entity.rigidBodyData.motionType = BodyMotionType::Dynamic;
                world.SpawnEntity( entity );
            }
        }

        // Player capsule - no render model, since it's viewed in first person.
        {
            Entity entity = MakeEntity( ENTITY_TYPE_PLAYER, { 0.0f, 3.0f, 10.0f } );
            playerId = world.SpawnEntity( entity );
        }

        // Other players are drawn as a cylinder standing in for a capsule
        {
            f32 height = 1.8f;
            f32 radius = 0.35f;
            MeshData capsuleData = Geometry::CreateCylinder( radius, height, 16, { 0.9f, 0.35f, 0.35f } );
            for ( Vertex & vertex : capsuleData.vertices ) {
                vertex.position.y += height * 0.5f;
            }
            capsuleMesh = UploadMesh( capsuleData );
        }

        // Small white "hands" box, drawn in front of every player
        {
            handMesh = UploadMesh( Geometry::CreateBox( 0.12f, 0.12f, 0.3f, { 1.0f, 1.0f, 1.0f } ) );
        }

        // Building - floor tiles and the walls that socket into their sides.
        // See tower_building.h for placement math/dimensions.
        {
            buildingFloorMesh = UploadMesh( Geometry::CreateBox( BuildingFloorSize, BuildingFloorThickness, BuildingFloorSize, { 0.65f, 0.5f, 0.35f } ) );
            buildingWallMesh = UploadMesh( Geometry::CreateBox( BuildingWallThickness, BuildingWallHeight, BuildingFloorSize, { 0.55f, 0.42f, 0.3f } ) );
        }

        font = std::make_unique<Font>( &Engine::Get().GetPermanentArena() );
        font->Load( "C:/Projects/2026/Sloth/assets/fonts/roboto/Roboto-Regular.ttf" ); // Hardcode for now

        SmallString address;
        address.Format( "127.0.0.1:%u", static_cast<u32>( tower::DefaultServerPort ) );
        serverConnection = network.Connect( address.Data() );
        SL_LOG_INFO( "TowerGame: connecting to server at %s", address.Data() );
    }

    void TowerGame::Shutdown() {
    }

    void TowerGame::Update( f32 deltaTime ) {
        camera.Update( deltaTime );
        UpdatePlayerMovement( deltaTime );
        UpdateShooting( deltaTime );
        UpdateBuilding();

        physicsWorld.Update( deltaTime );
        world.SyncPhysicsTransforms();

        if ( Entity * player = world.GetEntity( playerId ) ) {
            f32 yawRadians = glm::radians( camera.GetYaw() );
            player->rotation = glm::angleAxis( glm::half_pi<f32>() - yawRadians, glm::vec3( 0.0f, 1.0f, 0.0f ) );
        }

        UpdateNetworking();

        // Camera follows the player capsule's eye position and must run after SyncPhysicsTransforms() so entity.position reflects this frame's  physics step, not last frame's.
        if ( Entity * player = world.GetEntity( playerId ) ) {
            f32 eyeHeight = player->player.isCrouching ? player->player.crouchEyeHeight : player->player.eyeHeight;
            camera.SetPosition( player->position + glm::vec3( 0.0f, eyeHeight, 0.0f ) );
        }

        // Listener follows the camera, so 3D sounds (remote gunshots/hits)
        // pan and attenuate relative to where we're actually looking from.
        const Camera & cam = camera.GetCamera();
        audioWorld.SetListenerTransform( cam.GetPosition(), cam.GetForward(), cam.GetUp() );
        audioWorld.Update();

        world.FlushPendingChanges();
    }

    void TowerGame::UpdatePlayerMovement( f32 deltaTime ) {
        Entity * player = world.GetEntity( playerId );
        if ( player == nullptr || !player->character.IsValid() ) {
            return;
        }

        Input & input = Engine::Get().GetInput();

        bool wantsCrouch = input.IsKeyDown( Key::LeftControl );
        if ( wantsCrouch != player->player.isCrouching ) {
            f32 targetHeight = wantsCrouch ? player->player.crouchHeight : player->rigidBodyData.height;
            if ( physicsWorld.SetCharacterHeight( player->character, targetHeight, player->rigidBodyData.radius ) ) {
                player->player.isCrouching = wantsCrouch;
            }
        }

        f32 yawRadians = glm::radians( camera.GetYaw() );
        glm::vec3 flatForward = glm::normalize( glm::vec3( glm::cos( yawRadians ), 0.0f, glm::sin( yawRadians ) ) );
        glm::vec3 flatRight( -flatForward.z, 0.0f, flatForward.x );

        glm::vec3 movement( 0.0f );
        if ( input.IsKeyDown( Key::W ) ) movement += flatForward;
        if ( input.IsKeyDown( Key::S ) ) movement -= flatForward;
        if ( input.IsKeyDown( Key::D ) ) movement += flatRight;
        if ( input.IsKeyDown( Key::A ) ) movement -= flatRight;

        // Sprint is a hold, not a toggle, and is disabled while crouched
        // (crouching wins if both keys are held).
        bool sprinting = !player->player.isCrouching && input.IsKeyDown( Key::LeftShift );
        f32 moveSpeed = player->player.isCrouching ? player->player.crouchMoveSpeed
                       : sprinting                 ? player->player.sprintMoveSpeed
                                                    : player->player.moveSpeed;
        if ( glm::length( movement ) > 0.0f ) {
            movement = glm::normalize( movement ) * moveSpeed;
        }

        bool grounded = physicsWorld.IsCharacterGrounded( player->character );
        glm::vec3 currentVelocity = physicsWorld.GetCharacterLinearVelocity( player->character );

        f32 verticalVelocity;
        if ( grounded && !player->player.isCrouching && input.IsKeyPressed( Key::Space ) ) {
            verticalVelocity = player->player.jumpSpeed;
        } else if ( grounded ) {
            verticalVelocity = 0.0f;
        } else {
            verticalVelocity = currentVelocity.y - 9.81f * deltaTime;
        }

        physicsWorld.SetCharacterLinearVelocity( player->character, { movement.x, verticalVelocity, movement.z } );

        UpdateFootsteps( deltaTime, grounded, movement );
    }

    static constexpr f32 FootstepStrideLength = 2.2f; // Meters between steps - distance-based rather than a timer, so sprinting naturally steps faster and crouching slower.

    void TowerGame::UpdateFootsteps( f32 deltaTime, bool grounded, const glm::vec3 & desiredMovement ) {
        f32 speed = glm::length( desiredMovement );
        if ( !grounded || speed <= 0.0f ) {
            footstepDistance = 0.0f; // Don't bank distance while airborne/still, or landing would immediately fire a stale step.
            return;
        }

        footstepDistance += speed * deltaTime;
        if ( footstepDistance >= FootstepStrideLength ) {
            footstepDistance -= FootstepStrideLength;
            audioWorld.PlaySound2D( PickRandomSound( FootstepSounds, SL_ARRAY_COUNT( FootstepSounds ) ) );
        }
    }

    static constexpr f32 RecoilDuration = 0.15f;

    void TowerGame::UpdateShooting( f32 deltaTime ) {
        recoilTimer = std::max( 0.0f, recoilTimer - deltaTime );

        Entity * player = world.GetEntity( playerId );
        if ( player == nullptr || !player->character.IsValid() ) {
            return;
        }

        if ( !Engine::Get().GetInput().IsMouseButtonPressed( MouseButton::Left ) ) {
            return;
        }

        recoilTimer = RecoilDuration;

        // Played locally, immediately - zero-latency feedback rather than
        // waiting on a round trip. Other clients hear this shot via the
        // server's PlayerShotFired broadcast instead (see UpdateNetworking).
        audioWorld.PlaySound2D( PickRandomSound( GunshotSounds, SL_ARRAY_COUNT( GunshotSounds ) ) );

        const Camera & cam = camera.GetCamera();
        PlayerShotMessage shot;
        shot.origin = cam.GetPosition();
        shot.direction = cam.GetForward();
        network.SendMessage( serverConnection, &shot, sizeof( shot ), NetSendType::Reliable );
    }

    static constexpr f32 BuildRange = 8.0f;

    void TowerGame::UpdateBuilding() {
        Input & input = Engine::Get().GetInput();
        const Camera & cam = camera.GetCamera();

        if ( input.IsKeyPressed( Key::F ) ) {
            // Flat on whatever's under the crosshair (or a fixed distance
            // ahead if nothing's there within range), free to face any yaw -
            // not locked to a world grid. Basic implementation: always
            // placed level, no matching the hit surface's own slope/normal.
            RayCastHit hit;
            glm::vec3 position = physicsWorld.Raycast( cam.GetPosition(), cam.GetForward(), BuildRange, hit )
                ? hit.point
                : cam.GetPosition() + cam.GetForward() * BuildRange * 0.5f;
            position.y += BuildingFloorThickness * 0.5f;

            glm::quat rotation = glm::angleAxis( glm::radians( camera.GetYaw() ), glm::vec3( 0.0f, 1.0f, 0.0f ) );

            RenderModel floorModel { shader.get(), buildingFloorMesh.get() };
            PlaceFloor( world, position, rotation, floorModel );
        }

        if ( input.IsKeyPressed( Key::G ) ) {
            // Attach a wall to whichever floor - and which of its 4 sides -
            // is under the crosshair.
            RayCastHit hit;
            if ( !physicsWorld.Raycast( cam.GetPosition(), cam.GetForward(), BuildRange, hit ) ) {
                return;
            }

            EntityId floorId = world.FindEntityByRigidBody( hit.body );
            Entity * floorEntity = world.GetEntity( floorId );
            if ( floorEntity == nullptr || floorEntity->type != ENTITY_TYPE_BUILDING_FLOOR ) {
                return;
            }

            // Nearest side to the hit point, worked out in the floor's own
            // local space (undoing its position/rotation) - squared
            // comparison avoids needing an abs() import for just this.
            glm::vec3 localHit = glm::inverse( floorEntity->rotation ) * ( hit.point - floorEntity->position );
            bool xDominant = ( localHit.x * localHit.x ) > ( localHit.z * localHit.z );
            BuildingSide side = xDominant
                ? ( localHit.x > 0.0f ? BuildingSide::PosX : BuildingSide::NegX )
                : ( localHit.z > 0.0f ? BuildingSide::PosZ : BuildingSide::NegZ );

            RenderModel wallModel { shader.get(), buildingWallMesh.get() };
            PlaceWall( world, floorId, side, wallModel );
        }
    }

    void TowerGame::UpdateNetworking() {
        network.Update( Engine::Get().GetFrameArena() );

        for ( usize i = 0; i < network.GetEventCount(); i++ ) {
            const NetEvent & event = network.GetEvents()[i];
            if ( event.connection.Id != serverConnection.Id ) {
                continue; // not the server connection (shouldn't happen client-side, but be defensive)
            }

            switch ( event.type ) {
                case NetEventType::Connected:
                    SL_LOG_INFO( "TowerGame: connected to server" );
                    break;

                case NetEventType::Disconnected:
                    SL_LOG_INFO( "TowerGame: disconnected from server" );
                    hasLocalPlayerId = false;
                    break;

                case NetEventType::MessageReceived: {
                    if ( event.dataSize < sizeof( MessageType ) ) {
                        break;
                    }

                    MessageType type;
                    memcpy( &type, event.data, sizeof( MessageType ) );

                    if ( type == MessageType::Welcome && event.dataSize == sizeof( WelcomeMessage ) ) {
                        WelcomeMessage message;
                        memcpy( &message, event.data, sizeof( message ) );
                        localPlayerId = message.playerId;
                        hasLocalPlayerId = true;
                        SL_LOG_INFO( "TowerGame: assigned player id %u", localPlayerId );
                    } else if ( type == MessageType::WorldSnapshot && event.dataSize >= sizeof( WorldSnapshotHeader ) ) {
                        ApplyWorldSnapshot( event.data, event.dataSize );
                    } else if ( type == MessageType::PlayerHealth && event.dataSize == sizeof( PlayerHealthMessage ) ) {
                        PlayerHealthMessage message;
                        memcpy( &message, event.data, sizeof( message ) );
                        HandlePlayerHealth( message );
                    } else if ( type == MessageType::PlayerRespawn && event.dataSize == sizeof( PlayerRespawnMessage ) ) {
                        PlayerRespawnMessage message;
                        memcpy( &message, event.data, sizeof( message ) );
                        HandlePlayerRespawn( message );
                    } else if ( type == MessageType::PlayerShotFired && event.dataSize == sizeof( PlayerShotFiredMessage ) ) {
                        PlayerShotFiredMessage message;
                        memcpy( &message, event.data, sizeof( message ) );
                        audioWorld.PlaySound3D( PickRandomSound( GunshotSounds, SL_ARRAY_COUNT( GunshotSounds ) ), message.origin );
                    } else if ( type == MessageType::PlayerHitConfirm && event.dataSize == sizeof( PlayerHitConfirmMessage ) ) {
                        PlayerHitConfirmMessage message;
                        memcpy( &message, event.data, sizeof( message ) );
                        audioWorld.PlaySound3D( PickRandomSound( HitSounds, SL_ARRAY_COUNT( HitSounds ) ), message.position );
                    }
                } break;

                case NetEventType::IncomingConnection:
                    break; // server role only, shouldn't happen client-side
            }
        }

        // Report our own transform every frame
        if ( hasLocalPlayerId ) {
            if ( Entity * player = world.GetEntity( playerId ) ) {
                PlayerStateMessage message;
                message.position = player->position;
                message.rotation = player->rotation;
                network.SendMessage( serverConnection, &message, sizeof( message ), NetSendType::Unreliable );
            }
        }
    }

    void TowerGame::ApplyWorldSnapshot( const u8 * data, usize size ) {
        WorldSnapshotHeader header;
        memcpy( &header, data, sizeof( header ) );

        usize expectedSize = sizeof( WorldSnapshotHeader ) + static_cast<usize>( header.playerCount ) * sizeof( PlayerSnapshotEntry );
        if ( size != expectedSize ) {
            return; // malformed/truncated
        }

        std::vector<u32> seenIds;
        seenIds.reserve( header.playerCount );

        const u8 * cursor = data + sizeof( header );
        for ( u32 i = 0; i < header.playerCount; i++ ) {
            PlayerSnapshotEntry entry;
            memcpy( &entry, cursor, sizeof( entry ) );
            cursor += sizeof( entry );

            if ( entry.playerId == localPlayerId ) {
                continue; // that's us - not drawn, we're first-person
            }

            seenIds.push_back( entry.playerId );

            auto it = remotePlayers.find( entry.playerId );
            if ( it == remotePlayers.end() ) {
                Entity remote = MakeEntity( ENTITY_TYPE_REMOTE_PLAYER, entry.position );
                remote.rotation = entry.rotation;
                remote.renderModel = { shader.get(), capsuleMesh.get() };
                remote.remotePlayer.networkId = entry.playerId;
                remotePlayers[entry.playerId] = world.SpawnEntity( remote );
            } else if ( Entity * remoteEntity = world.GetEntity( it->second ) ) {
                remoteEntity->position = entry.position;
                remoteEntity->rotation = entry.rotation;
            }
        }

        // Anyone we knew about but wasn't in this snapshot has disconnected.
        for ( auto it = remotePlayers.begin(); it != remotePlayers.end(); ) {
            if ( std::find( seenIds.begin(), seenIds.end(), it->first ) == seenIds.end() ) {
                world.DestroyEntity( it->second );
                it = remotePlayers.erase( it );
            } else {
                ++it;
            }
        }
    }

    void TowerGame::HandlePlayerHealth( const PlayerHealthMessage & message ) {
        localHealth = message.health;
    }

    void TowerGame::HandlePlayerRespawn( const PlayerRespawnMessage & message ) {
        Entity * player = world.GetEntity( playerId );
        if ( player == nullptr || !player->character.IsValid() ) {
            return;
        }

        physicsWorld.SetCharacterPosition( player->character, message.position );
        physicsWorld.SetCharacterLinearVelocity( player->character, glm::vec3( 0.0f ) );

        player->position = message.position;
    }

    void TowerGame::Render() {
        Engine & engine = Engine::Get();
        Window & window = engine.GetWindow();

        glViewport( 0, 0, window.GetWidth(), window.GetHeight() );
        glClearColor( 0.08f, 0.08f, 0.1f, 1.0f );
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        glm::mat4 viewProjection = camera.GetCamera().GetViewProjectionMatrix();

        shader->Bind();
        shader->SetMat4( "uViewProjection", viewProjection );

        for ( const Entity & entity : world.GetEntities() ) {
            if ( entity.type == ENTITY_TYPE_INVALID ) {
                continue;
            }

            if ( !entity.renderModel.shader || !entity.renderModel.mesh ) {
                continue; // e.g. the player capsule, which has no mesh since it's viewed in first person.
            }

            glm::mat4 model = glm::translate( glm::mat4( 1.0f ), entity.position ) * glm::mat4_cast( entity.rotation ) * glm::scale( glm::mat4( 1.0f ), glm::vec3( entity.scale ) );
            shader->SetMat4( "uModel", model );
            entity.renderModel.mesh->Draw();
        }

        RenderPlayerHands();
        RenderHud();
    }

    void TowerGame::RenderHud() {
        Window & window = Engine::Get().GetWindow();
        f32 screenWidth = static_cast<f32>( window.GetWidth() );
        f32 screenHeight = static_cast<f32>( window.GetHeight() );
        glm::mat4 screenProjection = MakeScreenProjection( screenWidth, screenHeight );

        {
            constexpr f32 armLength = 8.0f;
            constexpr f32 thickness = 2.0f;
            glm::vec2 center { screenWidth * 0.5f, screenHeight * 0.5f };
            glm::vec4 color { 1.0f, 1.0f, 1.0f, 0.85f };

            guiRenderer.DrawRect( { center.x - armLength, center.y - thickness * 0.5f }, { center.x + armLength, center.y + thickness * 0.5f }, color );
            guiRenderer.DrawRect( { center.x - thickness * 0.5f, center.y - armLength }, { center.x + thickness * 0.5f, center.y + armLength }, color );
            guiRenderer.Flush( screenProjection );
        }

        // Health, bottom-right corner.
        if ( font->IsLoaded() ) {
            SmallString healthLabel;
            healthLabel.Format( "Health: %d", static_cast<i32>( localHealth ) );

            constexpr f32 pixelHeight = 24.0f;
            constexpr f32 margin = 16.0f;
            f32 textWidth = textRenderer.MeasureText( *font, glyphCache, healthLabel.View(), pixelHeight );
            glm::vec2 pos { screenWidth - margin - textWidth, screenHeight - margin }; // y is the text baseline, not its top - see DrawText's docs.

            textRenderer.DrawText( *font, glyphCache, healthLabel.View(), pos, pixelHeight, { 0.0f, 0.0f, 0.0f, 1.0f }, screenProjection );
        }
    }

    void TowerGame::RenderPlayerHands() {
        constexpr f32 handHeight = 1.3f;        // Above the feet - remote players only, see below.
        constexpr f32 handForwardOffset = 0.4f;
        constexpr f32 handRightOffset = 0.25f;  // Off-center to the right, like a held item, instead of floating dead-center.
        constexpr f32 handDownOffset = 0.2f;    // Local player only, to sit the box in the lower part of the view like a held item.

        for ( const Entity & entity : world.GetEntities() ) {
            if ( entity.type != ENTITY_TYPE_PLAYER && entity.type != ENTITY_TYPE_REMOTE_PLAYER ) {
                continue;
            }

            glm::mat4 model;
            if ( entity.id == playerId ) {
                constexpr f32 recoilPushback = 0.15f;
                f32 recoil = ( recoilTimer / RecoilDuration ) * recoilPushback;

                const Camera & cam = camera.GetCamera();
                glm::vec3 handPosition = cam.GetPosition() + cam.GetForward() * ( handForwardOffset - recoil ) + cam.GetRight() * handRightOffset - cam.GetUp() * handDownOffset;
                model = glm::mat4( glm::vec4( cam.GetRight(), 0.0f ), glm::vec4( cam.GetUp(), 0.0f ), glm::vec4( cam.GetForward(), 0.0f ), glm::vec4( handPosition, 1.0f ) );
            } else {
                glm::vec3 forward = entity.rotation * glm::vec3( 0.0f, 0.0f, 1.0f );
                // -X, not +X: with how player rotation is built from yaw in
                // Update(), local +X ends up pointing left, not right.
                glm::vec3 right = entity.rotation * glm::vec3( -1.0f, 0.0f, 0.0f );
                glm::vec3 handPosition = entity.position + glm::vec3( 0.0f, handHeight, 0.0f ) + forward * handForwardOffset + right * handRightOffset;
                model = glm::translate( glm::mat4( 1.0f ), handPosition ) * glm::mat4_cast( entity.rotation );
            }

            shader->SetMat4( "uModel", model );
            handMesh->Draw();
        }
    }

}
