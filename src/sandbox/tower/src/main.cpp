#include <core/sloth_engine.h>

#include <tower_game.h>

#include <GLFW/glfw3.h>

using namespace sloth;

int main() {
    WindowProps props;
    props.Title = "Sloth Engine - Tower";
    props.Width = 1280;
    props.Height = 720;

    Engine & engine = Engine::Get();
    engine.Init( props );

    Window & window = engine.GetWindow();

    tower::TowerGame game;
    game.Init();

    f64 lastFrameTime = glfwGetTime();

    while ( !window.ShouldClose() ) {
        if ( engine.GetInput().IsKeyPressed( Key::Escape ) == true ) {
            window.SetShouldClose( true );
        }

        f64 currentFrameTime = glfwGetTime();
        f32 deltaTime = static_cast<f32>( currentFrameTime - lastFrameTime );
        lastFrameTime = currentFrameTime;

        game.Update( deltaTime );
        game.Render();

        engine.EndFrame();
        window.OnUpdate();
    }

    game.Shutdown();
    engine.Shutdown();

    return 0;
}
