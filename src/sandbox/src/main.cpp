#include <glad/gl.h>
#include <core/sloth_engine.h>
#include <renderer/sloth_geometry.h>
#include <renderer/sloth_shader.h>
#include <renderer/sloth_static_mesh.h>

#include "FreeCamera.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

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

struct GeometryShowcaseEntry
{
    std::unique_ptr<StaticMesh> Mesh;
    glm::vec3 Position;
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

    std::vector<GeometryShowcaseEntry> shapes;
    shapes.push_back({ UploadMesh(Geometry::CreatePlane(2.0f, 2.0f, 4, 4, { 0.6f, 0.6f, 0.6f })), { -6.0f, 0.0f, 0.0f } });
    shapes.push_back({ UploadMesh(Geometry::CreateBox(1.5f, 1.5f, 1.5f, { 0.9f, 0.3f, 0.3f })), { -3.0f, 0.0f, 0.0f } });
    shapes.push_back({ UploadMesh(Geometry::CreateUVSphere(1.0f, 16, 32, { 0.3f, 0.9f, 0.3f })), { 0.0f, 0.0f, 0.0f } });
    shapes.push_back({ UploadMesh(Geometry::CreateIsoSphere(1.0f, 3, { 0.3f, 0.3f, 0.9f })), { 3.0f, 0.0f, 0.0f } });
    shapes.push_back({ UploadMesh(Geometry::CreateCylinder(0.75f, 2.0f, 24, { 0.9f, 0.9f, 0.3f })), { 6.0f, 0.0f, 0.0f } });

    FreeCamera freeCamera;
    freeCamera.GetCamera().SetPosition({ 0.0f, 2.0f, 10.0f });
    freeCamera.GetCamera().SetRotation(-90.0f, -10.0f);

    f64 lastFrameTime = glfwGetTime();

    while (!window.ShouldClose())
    {
        f64 currentFrameTime = glfwGetTime();
        f32 deltaTime = static_cast<f32>(currentFrameTime - lastFrameTime);
        lastFrameTime = currentFrameTime;

        freeCamera.Update(deltaTime);

        glClearColor(0.10f, 0.30f, 0.80f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (const GeometryShowcaseEntry& shape : shapes)
        {
            shader.SetMat4("uViewProjection", freeCamera.GetCamera().GetViewProjectionMatrix());
            shader.SetMat4("uModel", glm::translate(glm::mat4(1.0f), shape.Position));
            shape.Mesh->Draw();
        }

        engine.EndFrame();
        window.OnUpdate();
    }

    engine.Shutdown();

    return 0;
}
