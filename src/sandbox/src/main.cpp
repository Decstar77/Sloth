#include <glad/gl.h>
#include <core/sloth_engine.h>
#include <font/sloth_font.h>
#include <physics/sloth_physics_world.h>
#include <renderer/sloth_geometry.h>
#include <renderer/sloth_glyph_cache.h>
#include <renderer/sloth_shader.h>
#include <renderer/sloth_static_mesh.h>
#include <renderer/sloth_text_renderer.h>

#include "FreeCamera.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <vector>

using namespace sloth;

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

// Pairs a render mesh with the physics body driving its transform. The mesh
// itself never moves; each frame's model matrix is read back from
// PhysicsWorld after Update() so physics stays the single source of truth
// for position/rotation.
struct PhysicsEntry
{
    std::unique_ptr<StaticMesh> Mesh;
    RigidBody Body;
};

static std::unique_ptr<StaticMesh> UploadMesh(const MeshData& data)
{
    return std::make_unique<StaticMesh>(data.Vertices.data(), static_cast<u32>(data.Vertices.size()),
                                         data.Indices.data(), static_cast<u32>(data.Indices.size()));
}

int main()
{
    WindowProps props;
    props.Title = "Sloth Engine";
    props.Width = 1280;
    props.Height = 720;

    Engine& engine = Engine::Get();
    engine.Init(props);

    Window& window = engine.GetWindow();

    glEnable(GL_DEPTH_TEST);

    Shader shader(VertexShaderSource, FragmentShaderSource);

    Font font(&engine.GetPermanentArena());
    font.Load("../../assets/fonts/kenvector_future.ttf");
    GlyphCache glyphCache;
    TextRenderer textRenderer;

    PhysicsWorld physicsWorld;
    std::vector<PhysicsEntry> entries;

    // Static floor.
    {
        glm::vec3 halfExtents(10.0f, 0.5f, 10.0f);

        RigidBodyDesc desc;
        desc.Position = { 0.0f, -0.5f, 0.0f };
        desc.MotionType = BodyMotionType::Static;

        RigidBody body = physicsWorld.CreateBoxBody(halfExtents, desc);
        auto mesh = UploadMesh(Geometry::CreateBox(halfExtents.x * 2.0f, halfExtents.y * 2.0f, halfExtents.z * 2.0f,
                                                    { 0.5f, 0.5f, 0.55f }));
        entries.push_back({ std::move(mesh), body });
    }

    // Falling sphere.
    {
        RigidBodyDesc desc;
        desc.Position = { -1.5f, 8.0f, 0.0f };
        desc.Restitution = 0.4f;

        RigidBody body = physicsWorld.CreateSphereBody(1.0f, desc);
        auto mesh = UploadMesh(Geometry::CreateUVSphere(1.0f, 16, 32, { 0.3f, 0.9f, 0.3f }));
        entries.push_back({ std::move(mesh), body });
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
        entries.push_back({ std::move(mesh), body });
    }

    FreeCamera freeCamera;
    freeCamera.GetCamera().SetPosition({ 0.0f, 4.0f, 14.0f });
    freeCamera.GetCamera().SetRotation(-90.0f, -15.0f);

    f64 lastFrameTime = glfwGetTime();

    while (!window.ShouldClose())
    {
        f64 currentFrameTime = glfwGetTime();
        f32 deltaTime = static_cast<f32>(currentFrameTime - lastFrameTime);
        lastFrameTime = currentFrameTime;

        freeCamera.Update(deltaTime);
        physicsWorld.Update(deltaTime);

        glClearColor(0.10f, 0.30f, 0.80f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (const PhysicsEntry& entry : entries)
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), physicsWorld.GetPosition(entry.Body)) *
                               glm::mat4_cast(physicsWorld.GetRotation(entry.Body));

            shader.SetMat4("uViewProjection", freeCamera.GetCamera().GetViewProjectionMatrix());
            shader.SetMat4("uModel", model);
            entry.Mesh->Draw();
        }

        if (font.IsLoaded())
        {
            glm::mat4 textProjection = glm::ortho(0.0f, static_cast<f32>(window.GetWidth()), static_cast<f32>(window.GetHeight()), 0.0f);
            textRenderer.DrawText(font, glyphCache, "Hello, Sloth!", { 32.0f, 64.0f }, 48.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, textProjection);
        }

        engine.EndFrame();
        window.OnUpdate();
    }

    engine.Shutdown();

    return 0;
}
