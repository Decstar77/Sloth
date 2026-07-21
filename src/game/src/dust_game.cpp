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

            Entity entity = MakeEntity( ENTITY_TYPE_PROP, FACTION_TYPE_NEUTRAL, { 0.0f, -0.5f, 0.0f } );
            entity.renderModel = { shader.get(), floorMesh.get() };
            entity.rigidBodyData.shape = RigidBodyShape::Box;
            entity.rigidBodyData.halfExtents = halfExtents;
            entity.rigidBodyData.motionType = BodyMotionType::Static;
            world.SpawnEntity( entity );
        }

        // Player dune buggy.
        {
            VehicleData vehicleDefaults;
            glm::vec3 chassisHalfExtents = vehicleDefaults.chassisHalfExtents;

            buggyChassisMesh = UploadMesh( Geometry::CreateBox( chassisHalfExtents.x * 2.0f, chassisHalfExtents.y * 2.0f, chassisHalfExtents.z * 2.0f, { 0.85f, 0.5f, 0.15f } ) );
            buggyWheelMesh = UploadMesh( Geometry::CreateCylinder( vehicleDefaults.wheelRadius, vehicleDefaults.wheelWidth, 12, { 0.05f, 0.05f, 0.05f } ) );

            Entity entity = MakeEntity( ENTITY_TYPE_VEHICLE, FACTION_TYPE_PLAYER, { 0.0f, 3.0f, 0.0f } );
            entity.playerControlled = true;
            entity.renderModel = { shader.get(), buggyChassisMesh.get() };

            entity.rigidBodyData.shape = RigidBodyShape::Box;
            entity.rigidBodyData.halfExtents = chassisHalfExtents;
            entity.rigidBodyData.restitution = 0.1f;
            entity.rigidBodyData.motionType = BodyMotionType::Dynamic;
            entity.rigidBodyData.friction = 0.05f; // // Low, not zero: the chassis is a flat box directly touching the ground (no wheel model yet)
            entity.rigidBodyData.restitution = 0.05f;

            playerVehicleId = world.SpawnEntity( entity );
        }

        // AI dune buggy.
        {
            VehicleData vehicleDefaults;
            glm::vec3 chassisHalfExtents = vehicleDefaults.chassisHalfExtents;

            Entity entity = MakeEntity( ENTITY_TYPE_VEHICLE, FACTION_TYPE_REMNANT, { -3.0f, 3.0f, 0.0f } );
            entity.renderModel = { shader.get(), buggyChassisMesh.get() };

            entity.rigidBodyData.shape = RigidBodyShape::Box;
            entity.rigidBodyData.halfExtents = chassisHalfExtents;
            entity.rigidBodyData.restitution = 0.1f;
            entity.rigidBodyData.motionType = BodyMotionType::Dynamic;
            entity.rigidBodyData.friction = 0.05f; // Low, not zero: the chassis is a flat box directly touching the ground (no wheel model yet)
            entity.rigidBodyData.restitution = 0.05f;

            aiVehicleId = world.SpawnEntity( entity );
        }

        // Ore nodes, one of each type.
        {
            struct OreNodeSpawn {
                OreNodeType type;
                glm::vec3   color;
                glm::vec3   position;
            };

            const OreNodeSpawn spawns[] = {
                { ORE_NODE_TYPE_IRON,     { 0.70f, 0.70f, 0.35f }, {  14, 0, -14 } },
                { ORE_NODE_TYPE_COPPER,   { 0.80f, 0.45f, 0.20f }, {  26, 0, -14 } },
                { ORE_NODE_TYPE_COAL,     { 0.15f, 0.15f, 0.15f }, {  38, 0, -14 } },
                { ORE_NODE_TYPE_SULPHUR,  { 0.90f, 0.85f, 0.20f }, {  14, 0, -26 } },
                { ORE_NODE_TYPE_ALUMINUM, { 0.75f, 0.78f, 0.80f }, {  26, 0, -26 } },
                { ORE_NODE_TYPE_CHROME,   { 0.55f, 0.60f, 0.65f }, {  38, 0, -26 } },
            };

            glm::vec3 halfExtents( 4.0f, 2.3f, 4.0f );
            for ( usize i = 0; i < sizeof( spawns ) / sizeof( spawns[0] ); i++ ) {
                const OreNodeSpawn & spawn = spawns[i];
                oreNodeMeshes[i] = UploadMesh( Geometry::CreateBox( halfExtents.x * 2.0f, halfExtents.y * 2.0f, halfExtents.z * 2.0f, spawn.color ) );

                Entity entity = MakeEntity( ENTITY_TYPE_ORE_NODE, FACTION_TYPE_NEUTRAL, spawn.position );
                entity.renderModel = { shader.get(), oreNodeMeshes[i].get() };
                entity.oreNode.type = spawn.type;
                entity.oreNode.amount = 2000;

                entity.rigidBodyData.shape = RigidBodyShape::Box;
                entity.rigidBodyData.halfExtents = halfExtents;
                entity.rigidBodyData.motionType = BodyMotionType::Static;

                world.SpawnEntity( entity );
            }
        }

        // Shop
        {
            glm::vec3 halfExtents( 3.0f, 2.3f, 3.0f );
            shopMesh = UploadMesh( Geometry::CreateBox( halfExtents.x * 2.0f, halfExtents.y * 2.0f, halfExtents.z * 2.0f, { 0.3f, 0.7f, 0.3f } ) );

            Entity entity = MakeEntity( ENTITY_TYPE_SHOP, FACTION_TYPE_REMNANT, { -14, 0, 25 } );
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
        for ( auto & mesh : oreNodeMeshes ) {
            mesh.reset();
        }
        shopMesh.reset();
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
        if ( entity->playerControlled == false ) {
            return;
        }

        Input & input = Engine::Get().GetInput();

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
            world.ActionPlayerControl( entity );
        }

        DriveVehicle( physicsWorld, *entity, throttle, steer, deltaTime );
    }

    void DustGame::PlayerUpdateTargeting() {
        Input & input = Engine::Get().GetInput();
        if ( !input.IsMouseButtonPressed( MouseButton::Left ) ) {
            return;
        }

        Entity * player = world.GetEntity( playerVehicleId );
        if ( player == nullptr ) {
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
            Entity * targetEntity = world.GetEntity( hitId );
            SL_ASSERT( targetEntity );
            if ( targetEntity != nullptr ) {
                switch ( targetEntity->type ) {
                    case ENTITY_TYPE_PROP: { world.ActionIdle( player ); } break;
                    case ENTITY_TYPE_VEHICLE: { player->action.targetId = hitId; } break;
                    case ENTITY_TYPE_ORE_NODE: { world.ActionMineOre( player, hitId ); } break;
                    case ENTITY_TYPE_SHOP: { world.ActionSellOre( player, hitId ); } break;
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
        return world.GetPlayerCredits();
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
