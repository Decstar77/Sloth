#include "tower_game.h"

#include <core/sloth_engine.h>
#include <renderer/sloth_geometry.h>

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
    }

    void TowerGame::Shutdown() {
    }

    void TowerGame::Update( f32 deltaTime ) {
        camera.Update( deltaTime );
        UpdatePlayerMovement( deltaTime );

        physicsWorld.Update( deltaTime );
        world.SyncPhysicsTransforms();

        // Camera follows the player capsule's eye position - must run after
        // SyncPhysicsTransforms() so entity.position reflects this frame's
        // physics step, not last frame's.
        if ( Entity * player = world.GetEntity( playerId ) ) {
            camera.SetPosition( player->position + glm::vec3( 0.0f, player->player.eyeHeight, 0.0f ) );
        }

        world.FlushPendingChanges();
    }

    void TowerGame::UpdatePlayerMovement( f32 deltaTime ) {
        Entity * player = world.GetEntity( playerId );
        if ( player == nullptr || !player->rigidBody.IsValid() ) {
            return;
        }

        Input & input = Engine::Get().GetInput();

        // Walk relative to the camera's flat (yaw-only) facing direction, so
        // "forward" always means "the way you're looking" regardless of pitch.
        f32 yawRadians = glm::radians( camera.GetYaw() );
        glm::vec3 flatForward = glm::normalize( glm::vec3( glm::cos( yawRadians ), 0.0f, glm::sin( yawRadians ) ) );
        glm::vec3 flatRight( -flatForward.z, 0.0f, flatForward.x );

        glm::vec3 movement( 0.0f );
        if ( input.IsKeyDown( Key::W ) ) movement += flatForward;
        if ( input.IsKeyDown( Key::S ) ) movement -= flatForward;
        if ( input.IsKeyDown( Key::D ) ) movement += flatRight;
        if ( input.IsKeyDown( Key::A ) ) movement -= flatRight;

        if ( glm::length( movement ) > 0.0f ) {
            movement = glm::normalize( movement ) * player->player.moveSpeed;
        }

        // Only drive the horizontal velocity directly - the vertical
        // component is left alone so gravity (and, later, jumping) keeps
        // working through the physics simulation rather than being
        // overwritten every frame.
        glm::vec3 currentVelocity = physicsWorld.GetLinearVelocity( player->rigidBody );
        physicsWorld.SetLinearVelocity( player->rigidBody, { movement.x, currentVelocity.y, movement.z } );
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
