#include <glad/gl.h>
#include <core/sloth_engine.h>

using namespace sloth;

int main()
{
    WindowProps props;
    props.Title = "Sloth Engine";
    props.Width = 1280;
    props.Height = 720;

    Engine& engine = Engine::Get();
    engine.Init(props);

    Window& window = engine.GetWindow();

    while (!window.ShouldClose())
    {
        glClearColor(0.10f, 0.30f, 0.80f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        window.OnUpdate();
        engine.EndFrame();
    }

    engine.Shutdown();

    return 0;
}
