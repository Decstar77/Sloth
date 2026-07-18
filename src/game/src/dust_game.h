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

        DustCamera&             GetCamera() { return camera; }
        sloth::PhysicsWorld&    GetPhysicsWorld() { return physicsWorld; }
        DustWorld&              GetWorld() { return world; }

    private:
        void                    UpdateVehicleControl(f32 deltaTime);
        void                    DrawVehicle(const Entity& entity, const glm::mat4& viewProjection);

    private:
        DustWorld                       world;
        DustCamera                      camera;

        std::unique_ptr<sloth::Shader>  shader;

        // Meshes shared across entities via RenderModel; owned here since
        // Entity/RenderModel only hold non-owning pointers to them.
        std::unique_ptr<sloth::StaticMesh> floorMesh;
        std::unique_ptr<sloth::StaticMesh> sphereMesh;
        std::unique_ptr<sloth::StaticMesh> boxMesh;

        // Buggy chassis/wheel meshes. Wheels are drawn per-entity from
        // VehicleData's offsets, not stored as separate entities.
        std::unique_ptr<sloth::StaticMesh> buggyChassisMesh;
        std::unique_ptr<sloth::StaticMesh> buggyWheelMesh;

        EntityId                        playerVehicleId = INVALID_ENTITY_ID;

        sloth::PhysicsWorld             physicsWorld;
    };

}
