#include "dust_game.h"

#include <renderer/sloth_geometry.h>

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace sloth;

namespace dust {

    static const char* VertexShaderSource = R"(
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

    static const char* FragmentShaderSource = R"(
        #version 450 core
        in vec3 vColor;
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(vColor, 1.0);
        }
    )";

    static std::unique_ptr<StaticMesh> UploadMesh(const MeshData& data)
    {
        return std::make_unique<StaticMesh>(data.Vertices.data(), static_cast<u32>(data.Vertices.size()),
                                             data.Indices.data(), static_cast<u32>(data.Indices.size()));
    }

    void DustGame::Init()
    {
        glEnable(GL_DEPTH_TEST);

        world.Init(&physicsWorld);

        shader = std::make_unique<Shader>(VertexShaderSource, FragmentShaderSource);

        // Static floor.
        {
            glm::vec3 halfExtents(10.0f, 0.5f, 10.0f);

            RigidBodyDesc desc;
            desc.Position = { 0.0f, -0.5f, 0.0f };
            desc.MotionType = BodyMotionType::Static;

            RigidBody body = physicsWorld.CreateBoxBody(halfExtents, desc);
            auto mesh = UploadMesh(Geometry::CreateBox(halfExtents.x * 2.0f, halfExtents.y * 2.0f, halfExtents.z * 2.0f,
                                                        { 0.5f, 0.5f, 0.55f }));
            physicsEntries.push_back({ std::move(mesh), body });
        }

        // Falling sphere.
        {
            RigidBodyDesc desc;
            desc.Position = { -1.5f, 8.0f, 0.0f };
            desc.Restitution = 0.4f;

            RigidBody body = physicsWorld.CreateSphereBody(1.0f, desc);
            auto mesh = UploadMesh(Geometry::CreateUVSphere(1.0f, 16, 32, { 0.3f, 0.9f, 0.3f }));
            physicsEntries.push_back({ std::move(mesh), body });
        }

        // Falling, tumbling box.
        {
            glm::vec3 halfExtents(0.75f, 0.75f, 0.75f);

            RigidBodyDesc desc;
            desc.Position = { 1.5f, 11.0f, 0.0f };
            desc.Rotation = glm::angleAxis(glm::radians(25.0f), glm::normalize(glm::vec3(1.0f, 0.5f, 0.0f)));
            desc.Restitution = 0.1f;

            RigidBody body = physicsWorld.CreateBoxBody(halfExtents, desc);
            auto mesh = UploadMesh(Geometry::CreateBox(halfExtents.x * 2.0f, halfExtents.y * 2.0f, halfExtents.z * 2.0f,
                                                        { 0.9f, 0.3f, 0.3f }));
            physicsEntries.push_back({ std::move(mesh), body });
        }

        camera.SetFocusPoint({ 0.0f, 0.0f, 0.0f });
        camera.SetDistance(20.0f);
    }

    void DustGame::Shutdown()
    {
        physicsEntries.clear();
        shader.reset();
    }

    void DustGame::Update(f32 deltaTime)
    {
        camera.Update(deltaTime);
        physicsWorld.Update(deltaTime);

        // Entity spawn/destroy requests buffered this frame are applied once
        // here, at the end of the frame's update.
        world.FlushPendingChanges();
    }

    void DustGame::Render()
    {
        glClearColor(0.10f, 0.30f, 0.80f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 viewProjection = camera.GetCamera().GetViewProjectionMatrix();

        for (const PhysicsEntry& entry : physicsEntries)
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), physicsWorld.GetPosition(entry.Body)) *
                               glm::mat4_cast(physicsWorld.GetRotation(entry.Body));

            shader->SetMat4("uViewProjection", viewProjection);
            shader->SetMat4("uModel", model);
            entry.Mesh->Draw();
        }
    }

}
