#include "dust_game.h"

#include <core/sloth_engine.h>
#include <renderer/sloth_debug_renderer.h>
#include <renderer/sloth_geometry.h>

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace sloth;

namespace dust {

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

    void DustGame::Init() {
        glEnable( GL_DEPTH_TEST );

        world.Init( &physicsWorld );

        shader = std::make_unique<Shader>( VertexShaderSource, FragmentShaderSource );

        // Static floor.
        {
            glm::vec3 halfExtents( 70.0f, 0.5f, 70.0f );
            floorMesh = UploadMesh( Geometry::CreateBox( halfExtents.x * 2.0f, halfExtents.y * 2.0f, halfExtents.z * 2.0f, { 0.5f, 0.5f, 0.55f } ) );

            Entity entity = MakeEntity( ENTITY_TYPE_PROP, { 0.0f, -0.5f, 0.0f } );
            entity.renderModel = { shader.get(), floorMesh.get() };
            entity.rigidBodyData.shape = RigidBodyShape::Box;
            entity.rigidBodyData.halfExtents = halfExtents;
            entity.rigidBodyData.motionType = BodyMotionType::Static;
            world.SpawnEntity( entity );
        }

        // Falling sphere.
        {
            sphereMesh = UploadMesh( Geometry::CreateUVSphere( 1.0f, 16, 32, { 0.3f, 0.9f, 0.3f } ) );

            Entity entity = MakeEntity( ENTITY_TYPE_PROP, { -1.5f, 8.0f, 0.0f } );
            entity.renderModel = { shader.get(), sphereMesh.get() };
            entity.rigidBodyData.shape = RigidBodyShape::Sphere;
            entity.rigidBodyData.radius = 1.0f;
            entity.rigidBodyData.restitution = 0.4f;
            world.SpawnEntity( entity );
        }

        // Falling, tumbling box.
        {
            glm::vec3 halfExtents( 0.75f, 0.75f, 0.75f );
            boxMesh = UploadMesh( Geometry::CreateBox( halfExtents.x * 2.0f, halfExtents.y * 2.0f, halfExtents.z * 2.0f, { 0.9f, 0.3f, 0.3f } ) );

            Entity entity = MakeEntity( ENTITY_TYPE_PROP, { 1.5f, 11.0f, 0.0f } );
            entity.renderModel = { shader.get(), boxMesh.get() };
            entity.rotation = glm::angleAxis( glm::radians( 25.0f ), glm::normalize( glm::vec3( 1.0f, 0.5f, 0.0f ) ) );
            entity.rigidBodyData.shape = RigidBodyShape::Box;
            entity.rigidBodyData.halfExtents = halfExtents;
            entity.rigidBodyData.restitution = 0.1f;
            world.SpawnEntity( entity );
        }

        // Player dune buggy.
        {
            VehicleData vehicleDefaults;
            glm::vec3 chassisHalfExtents = vehicleDefaults.chassisHalfExtents;

            buggyChassisMesh = UploadMesh( Geometry::CreateBox( chassisHalfExtents.x * 2.0f, chassisHalfExtents.y * 2.0f, chassisHalfExtents.z * 2.0f, { 0.85f, 0.5f, 0.15f } ) );
            buggyWheelMesh = UploadMesh( Geometry::CreateCylinder( vehicleDefaults.wheelRadius, vehicleDefaults.wheelWidth, 12, { 0.05f, 0.05f, 0.05f } ) );

            Entity entity = MakeEntity( ENTITY_TYPE_VEHICLE, { 0.0f, 3.0f, 0.0f } );
            entity.renderModel = { shader.get(), buggyChassisMesh.get() };
            entity.vehicle.playerControlled = true;

            entity.rigidBodyData.shape = RigidBodyShape::Box;
            entity.rigidBodyData.halfExtents = chassisHalfExtents;
            entity.rigidBodyData.restitution = 0.1f;
            entity.rigidBodyData.motionType = BodyMotionType::Dynamic;
            entity.rigidBodyData.friction = 0.05f; // // Low, not zero: the chassis is a flat box directly touching the ground (no wheel model yet)
            entity.rigidBodyData.restitution = 0.05f;

            playerVehicleId = world.SpawnEntity( entity );
        }

        // Iron ore node
        {
            glm::vec3 halfExtents( 4.0f, 2.3f, 4.0f );
            oreNodeMesh = UploadMesh( Geometry::CreateBox( halfExtents.x * 2.0f, halfExtents.y * 2.0f, halfExtents.z * 2.0f, { 0.7f, 0.7f, 0.35f } ) );

            Entity entity = MakeEntity( ENTITY_TYPE_ORE_NODE, { 14, 0, -14 } );
            entity.renderModel = { shader.get(), oreNodeMesh.get() };
            entity.oreNode.type = ORE_NODE_TYPE_IRON;
            entity.oreNode.amount = 2000;

            entity.rigidBodyData.shape = RigidBodyShape::Box;
            entity.rigidBodyData.halfExtents = halfExtents;
            entity.rigidBodyData.motionType = BodyMotionType::Static;

            world.SpawnEntity( entity );
        }

        // Shop
        {
            glm::vec3 halfExtents( 3.0f, 2.3f, 3.0f );
            shopMesh = UploadMesh( Geometry::CreateBox( halfExtents.x * 2.0f, halfExtents.y * 2.0f, halfExtents.z * 2.0f, { 0.3f, 0.7f, 0.3f } ) );

            Entity entity = MakeEntity( ENTITY_TYPE_SHOP, { -14, 0, 25 } );
            entity.renderModel = { shader.get(), shopMesh.get() };
            entity.shop.credits = 1000;

            entity.rigidBodyData.shape = RigidBodyShape::Box;
            entity.rigidBodyData.halfExtents = halfExtents;
            entity.rigidBodyData.motionType = BodyMotionType::Static;

            world.SpawnEntity( entity );
        }

        camera.SetFocusPoint( { 0.0f, 0.0f, 0.0f } );
        camera.SetDistance( 20.0f );
    }

    void DustGame::Shutdown() {
        floorMesh.reset();
        sphereMesh.reset();
        boxMesh.reset();
        buggyChassisMesh.reset();
        buggyWheelMesh.reset();
        shader.reset();
    }

    void DustGame::Update( f32 deltaTime ) {
        camera.Update( deltaTime );

        PlayerUpdateVehicleControl( deltaTime );
        PlayerUpdateTargeting();
        world.Update( deltaTime );
        physicsWorld.Update( deltaTime );
        world.SyncPhysicsTransforms();

        // Camera follows the player vehicle: overrides the manual WASD pan
        // camera.Update() just computed, since those keys now drive the
        // buggy instead.
        if ( Entity * vehicle = world.GetEntity( playerVehicleId ) ) {
            camera.SetFocusPoint( vehicle->position );
        }

        // Entity spawn/destroy requests buffered this frame are applied once
        // here, at the end of the frame's update.
        world.FlushPendingChanges();
    }

    void DustGame::PlayerUpdateVehicleControl( f32 deltaTime ) {
        Entity * entity = world.GetEntity( playerVehicleId );
        if ( !entity || entity->type != ENTITY_TYPE_VEHICLE || !entity->rigidBody.IsValid() ) {
            return;
        }

        VehicleData & vehicle = entity->vehicle;
        if ( !vehicle.playerControlled ) {
            return;
        }

        Input & input = Engine::Get().GetInput();

        glm::vec3 forward = entity->rotation * glm::vec3( 0.0f, 0.0f, 1.0f );
        glm::vec3 right = entity->rotation * glm::vec3( 1.0f, 0.0f, 0.0f );
        glm::vec3 up = entity->rotation * glm::vec3( 0.0f, 1.0f, 0.0f );

        glm::vec3 velocity = physicsWorld.GetLinearVelocity( entity->rigidBody );
        f32 forwardSpeed = glm::dot( velocity, forward );

        bool hadInput = false;
        f32 throttle = 0.0f;
        if ( input.IsKeyDown( Key::W ) ) {
            throttle += 1.0f;
            hadInput = true;
        }
        if ( input.IsKeyDown( Key::S ) ) {
            throttle -= 1.0f;
            hadInput = true;
        }

        f32 steer = 0.0f;
        if ( input.IsKeyDown( Key::A ) ) {
            steer += 1.0f;
            hadInput = true;
        }
        if ( input.IsKeyDown( Key::D ) ) {
            steer -= 1.0f;
            hadInput = true;
        }

        if ( hadInput == true ) {
            entity->action.type = ENTITY_ACTION_TYPE_PLAYER_CONTROL;
            entity->action.progress = 0;
        }

        if ( throttle != 0.0f && glm::abs( forwardSpeed ) < vehicle.maxSpeed ) {
            physicsWorld.AddForce( entity->rigidBody, forward * throttle * vehicle.enginePower );
        }

        // Steering torque; flips sign in reverse like a real car backing up.
        // Not scaled down at low speed: the chassis is a flat box resting
        // directly on the ground (no wheels), so its own yaw friction
        // already resists spinning in place — a speed-scaled torque on top
        // of that was enough to fully cancel out at low speed and left
        // steering doing nothing. Turn rate is capped below instead.
        if ( steer != 0.0f ) {
            f32 reverseSign = forwardSpeed < 0.0f ? -1.0f : 1.0f;
            physicsWorld.AddTorque( entity->rigidBody, up * steer * reverseSign * vehicle.turnTorque );
        }

        // Clamp yaw spin rate so the steering torque above (sized to
        // overcome ground friction) doesn't turn the buggy into a spinning
        // top once it gets going.
        {
            glm::vec3 angularVelocity = physicsWorld.GetAngularVelocity( entity->rigidBody );
            f32 yawRate = glm::dot( angularVelocity, up );
            f32 clampedYawRate = glm::clamp( yawRate, -vehicle.maxYawRateRadians, vehicle.maxYawRateRadians );
            if ( clampedYawRate != yawRate ) {
                physicsWorld.SetAngularVelocity( entity->rigidBody, angularVelocity + up * ( clampedYawRate - yawRate ) );
            }
        }

        // Arcade tire grip: cancel most sideways velocity each frame so the
        // buggy corners instead of sliding around like a hockey puck. Stands
        // in for real wheel friction until there's an actual wheel model.
        f32 lateralSpeed = glm::dot( velocity, right );
        f32 gripFactor = glm::clamp( vehicle.gripStrength * deltaTime, 0.0f, 1.0f );
        physicsWorld.SetLinearVelocity( entity->rigidBody, velocity - right * lateralSpeed * gripFactor );

        // Visual-only wheel state, consumed by DrawVehicle().
        vehicle.steerAngleDegrees = glm::mix( vehicle.steerAngleDegrees, steer * vehicle.maxSteerAngleDegrees, glm::clamp( 8.0f * deltaTime, 0.0f, 1.0f ) );
        vehicle.wheelSpinRadians += ( forwardSpeed / glm::max( vehicle.wheelRadius, 0.01f ) ) * deltaTime;
    }

    void DustGame::PlayerUpdateTargeting() {
        Input & input = Engine::Get().GetInput();
        if ( !input.IsMouseButtonPressed( MouseButton::Left ) ) {
            return;
        }

        Entity * player = world.GetEntity( playerVehicleId );
        if ( !player ) {
            return;
        }

        Window & window = Engine::Get().GetWindow();

        glm::vec3 rayOrigin, rayDirection;
        camera.GetCamera().ScreenPointToRay(
            static_cast<f32>( input.GetMouseX() ), static_cast<f32>( input.GetMouseY() ),
            static_cast<f32>( window.GetWidth() ), static_cast<f32>( window.GetHeight() ),
            rayOrigin, rayDirection );

        constexpr f32 maxRayDistance = 500.0f;
        RayCastHit hit;
        if ( !physicsWorld.Raycast( rayOrigin, rayDirection, maxRayDistance, hit ) ) {
            return;
        }

        EntityId hitId = world.FindEntityByRigidBody( hit.body );
        if ( hitId != INVALID_ENTITY_ID ) {
            player->action.targetId = hitId;

            Entity * targetEntity = world.GetEntity( hitId );
            SL_ASSERT( targetEntity );
            if ( targetEntity != nullptr ) {
                switch ( targetEntity->type ) {
                    case ENTITY_TYPE_ORE_NODE: { player->action.type = ENTITY_ACTION_TYPE_MINING_ORE; } break;
                    case ENTITY_TYPE_SHOP: { player->action.type = ENTITY_ACTION_TYPE_SELL_ORE; } break;
                }
            }
        }
    }

    const Entity * DustGame::GetPlayer() const {
        const Entity * player = world.GetEntity( playerVehicleId );
        if ( !player ) {
            return nullptr;
        }

        return player;
    }

    const Entity * DustGame::GetPlayerTarget() const {
        const Entity * player = world.GetEntity( playerVehicleId );
        if ( !player ) {
            return nullptr;
        }

        return world.GetEntity( player->action.targetId );
    }

    i64 DustGame::GetPlayerCredits() const {
        return world.playerCredits;
    }

    void DustGame::Render() {
        glClearColor( 0.10f, 0.30f, 0.80f, 1.0f );
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        glm::mat4 viewProjection = camera.GetCamera().GetViewProjectionMatrix();

        for ( const Entity & entity : world.GetEntities() ) {
            if ( !entity.renderModel.shader || !entity.renderModel.mesh ) {
                continue;
            }

            if ( entity.type == ENTITY_TYPE_VEHICLE ) {
                DrawVehicle( entity, viewProjection );
            } else {
                // Generic draw
                glm::mat4 model = glm::translate( glm::mat4( 1.0f ), entity.position ) * glm::mat4_cast( entity.rotation );
                entity.renderModel.shader->SetMat4( "uViewProjection", viewProjection );
                entity.renderModel.shader->SetMat4( "uModel", model );
                entity.renderModel.mesh->Draw();
            }

            if ( entity.action.targetId != INVALID_ENTITY_ID ) {
                if ( const Entity * target = world.GetEntity( entity.action.targetId ) ) {
                    DebugRenderer::Get().DrawLine( entity.position, target->position, { 1.0f, 0.1f, 0.1f } );
                }
            }
        }

        DebugRenderer::Get().Render( viewProjection );
    }

    void DustGame::DrawVehicle( const Entity & entity, const glm::mat4 & viewProjection ) {
        if ( !buggyWheelMesh ) {
            return;
        }

        glm::mat4 chassisModel = glm::translate( glm::mat4( 1.0f ), entity.position ) * glm::mat4_cast( entity.rotation );

        entity.renderModel.shader->SetMat4( "uViewProjection", viewProjection );
        entity.renderModel.shader->SetMat4( "uModel", chassisModel );
        entity.renderModel.mesh->Draw();

        const VehicleData & vehicle = entity.vehicle;
        for ( i32 i = 0; i < 4; ++i ) {
            bool isFrontWheel = i < 2;
            f32 steerRadians = isFrontWheel ? glm::radians( vehicle.steerAngleDegrees ) : 0.0f;

            // Order (applied right-to-left): align the cylinder's default
            // Y-axis to the wheel's roll axis (X), spin it around that axis,
            // steer front wheels about the chassis' up axis, then place it.
            glm::mat4 wheelLocal = glm::translate( glm::mat4( 1.0f ), vehicle.wheelOffsets[i] ) * glm::rotate( glm::mat4( 1.0f ), steerRadians, glm::vec3( 0.0f, 1.0f, 0.0f ) ) * glm::rotate( glm::mat4( 1.0f ), vehicle.wheelSpinRadians, glm::vec3( 1.0f, 0.0f, 0.0f ) ) * glm::rotate( glm::mat4( 1.0f ), glm::radians( 90.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );

            entity.renderModel.shader->SetMat4( "uModel", chassisModel * wheelLocal );
            buggyWheelMesh->Draw();
        }
    }

} // namespace dust
