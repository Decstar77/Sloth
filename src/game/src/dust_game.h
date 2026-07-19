#pragma once

#include <core/sloth_defines.h>
#include <physics/sloth_physics_world.h>
#include <renderer/sloth_shader.h>
#include <renderer/sloth_static_mesh.h>

#include "dust_camera.h"
#include "dust_world.h"

#include <memory>
#include <vector>

namespace dust {

    class DustGame {
    public:
        void                    Init();
        void                    Shutdown();

        void                    Update(f32 deltaTime);
        void                    Render();

        DustCamera &            GetCamera() { return camera; }
        sloth::PhysicsWorld &   GetPhysicsWorld() { return physicsWorld; }
        DustWorld &             GetWorld() { return world; }

        const Entity *          GetPlayer() const;
        const Entity *          GetPlayerTarget() const;
        i64                     GetPlayerCredits() const;

    private:
        void                    PlayerUpdateVehicleControl( f32 deltaTime );
        void                    PlayerUpdateTargeting();
        void                    DrawVehicle( const Entity & entity, const glm::mat4 & viewProjection );

    private:
        DustWorld                       world;
        DustCamera                      camera;

        // Rendering stuff, need a better place to put all this
        std::unique_ptr<sloth::Shader>      shader;
        std::unique_ptr<sloth::StaticMesh>  floorMesh;
        std::unique_ptr<sloth::StaticMesh>  sphereMesh;
        std::unique_ptr<sloth::StaticMesh>  boxMesh;
        std::unique_ptr<sloth::StaticMesh>  buggyChassisMesh;
        std::unique_ptr<sloth::StaticMesh>  buggyWheelMesh;
        std::unique_ptr<sloth::StaticMesh>  oreNodeMeshes[6];
        std::unique_ptr<sloth::StaticMesh>  shopMesh;

        EntityId                        playerVehicleId = INVALID_ENTITY_ID;
        EntityId                        aiVehicleId = INVALID_ENTITY_ID;

        sloth::PhysicsWorld             physicsWorld;
    };

}

