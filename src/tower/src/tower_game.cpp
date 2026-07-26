#include "tower_game.h"

#include <core/sloth_engine.h>
#include <renderer/sloth_geometry.h>

#include "tower_server.h" // for DefaultServerPort - see tower_server.h

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

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
        UpdateNetworking();

        physicsWorld.Update( deltaTime );
        world.SyncPhysicsTransforms();

        // Camera follows the player capsule's eye position - must run after
        // SyncPhysicsTransforms() so entity.position reflects this frame's
        // physics step, not last frame's.
        if ( Entity * player = world.GetEntity( playerId ) ) {
            f32 eyeHeight = player->player.isCrouching ? player->player.crouchEyeHeight : player->player.eyeHeight;
            camera.SetPosition( player->position + glm::vec3( 0.0f, eyeHeight, 0.0f ) );
        }

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
                    break;

                case NetEventType::MessageReceived:
                    // No wire protocol defined yet - see TowerServer::HandleNetworkEvents.
                    break;

                case NetEventType::IncomingConnection:
                    break; // server role only, shouldn't happen client-side
            }
        }
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
    }

}
