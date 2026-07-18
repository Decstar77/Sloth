#include <glad/gl.h>
#include <core/sloth_engine.h>
#include <renderer/sloth_shader.h>
#include <renderer/sloth_static_mesh.h>

#include "FreeCamera.h"

#include <GLFW/glfw3.h>

using namespace sloth;

static const char* TriangleVertexShaderSource = R"(
    #version 450 core
    layout(location = 0) in vec3 aPosition;
    layout(location = 1) in vec3 aColor;

    uniform mat4 uViewProjection;

    out vec3 vColor;

    void main()
    {
        vColor = aColor;
        gl_Position = uViewProjection * vec4(aPosition, 1.0);
    }
)";

static const char* TriangleFragmentShaderSource = R"(
    #version 450 core
    in vec3 vColor;
    out vec4 FragColor;

    void main()
    {
        FragColor = vec4(vColor, 1.0);
    }
)";

int main()
{
    WindowProps props;
    props.Title = "Sloth Engine";
    props.Width = 1280;
    props.Height = 720;

    Engine& engine = Engine::Get();
    engine.Init(props);

    Window& window = engine.GetWindow();

    Shader triangleShader(TriangleVertexShaderSource, TriangleFragmentShaderSource);

    Vertex triangleVertices[] = {
        { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
    };
    u32 triangleIndices[] = { 0, 1, 2 };

    StaticMesh triangleMesh(triangleVertices, SL_ARRAY_COUNT(triangleVertices), triangleIndices, SL_ARRAY_COUNT(triangleIndices));

    FreeCamera freeCamera;
    freeCamera.GetCamera().SetPosition({ 0.0f, 0.0f, 3.0f });
    freeCamera.GetCamera().SetRotation(-90.0f, 0.0f);

    f64 lastFrameTime = glfwGetTime();

    while (!window.ShouldClose())
    {
        f64 currentFrameTime = glfwGetTime();
        f32 deltaTime = static_cast<f32>(currentFrameTime - lastFrameTime);
        lastFrameTime = currentFrameTime;

        freeCamera.Update(deltaTime);

        glClearColor(0.10f, 0.30f, 0.80f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        triangleShader.SetMat4("uViewProjection", freeCamera.GetCamera().GetViewProjectionMatrix());
        triangleMesh.Draw();

        window.OnUpdate();
        engine.EndFrame();
    }

    engine.Shutdown();

    return 0;
}
