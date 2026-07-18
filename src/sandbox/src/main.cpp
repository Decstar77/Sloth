#include <core/sloth_engine.h>
#include <font/sloth_font.h>
#include <renderer/sloth_glyph_cache.h>
#include <renderer/sloth_text_renderer.h>

#include <dust_game.h>

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

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

    Font font(&engine.GetPermanentArena());
    font.Load("../../assets/fonts/kenvector_future.ttf");
    GlyphCache glyphCache;
    TextRenderer textRenderer;

    dust::DustGame game;
    game.Init();

    f64 lastFrameTime = glfwGetTime();

    while (!window.ShouldClose())
    {
        f64 currentFrameTime = glfwGetTime();
        f32 deltaTime = static_cast<f32>(currentFrameTime - lastFrameTime);
        lastFrameTime = currentFrameTime;

        game.Update(deltaTime);
        game.Render();

        if (font.IsLoaded())
        {
            glm::mat4 textProjection = glm::ortho(0.0f, static_cast<f32>(window.GetWidth()), static_cast<f32>(window.GetHeight()), 0.0f);
            textRenderer.DrawText(font, glyphCache, "Hello, Sloth!", { 32.0f, 64.0f }, 18.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, textProjection);
        }

        engine.EndFrame();
        window.OnUpdate();
    }

    game.Shutdown();
    engine.Shutdown();

    return 0;
}
