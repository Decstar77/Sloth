#include <core/sloth_engine.h>
#include <font/sloth_font.h>
#include <renderer/sloth_glyph_cache.h>
#include <renderer/sloth_gui_renderer.h>
#include <renderer/sloth_text_renderer.h>
#include <renderer/sloth_texture.h>

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
    GuiRenderer guiRenderer;

    // Small in-memory checkerboard, exercising Texture's raw-pixel
    // constructor (no image file needed) - a stand-in for a real icon.
    constexpr i32 checkerSize = 16;
    u8 checkerPixels[checkerSize * checkerSize * 4];
    for (i32 y = 0; y < checkerSize; ++y)
    {
        for (i32 x = 0; x < checkerSize; ++x)
        {
            bool isLight = ((x / 4) + (y / 4)) % 2 == 0;
            u8* pixel = &checkerPixels[(y * checkerSize + x) * 4];
            pixel[0] = isLight ? 255 : 40;
            pixel[1] = isLight ? 210 : 40;
            pixel[2] = isLight ? 90 : 60;
            pixel[3] = 255;
        }
    }
    Texture checkerTexture(checkerPixels, checkerSize, checkerSize, TextureFilter::Nearest);

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

        glm::mat4 screenProjection = MakeScreenProjection(static_cast<f32>(window.GetWidth()), static_cast<f32>(window.GetHeight()));

        guiRenderer.DrawRect({ 32.0f, 96.0f }, { 320.0f, 280.0f }, { 0.15f, 0.15f, 0.18f, 0.9f }, 12.0f);
        guiRenderer.DrawRect({ 48.0f, 112.0f }, { 140.0f, 144.0f }, { 0.2f, 0.55f, 0.9f, 1.0f }, 6.0f);
        guiRenderer.DrawRect({ 48.0f, 160.0f }, { 140.0f, 192.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, 6.0f, 2.0f, { 0.9f, 0.9f, 0.9f, 1.0f });
        guiRenderer.DrawCircle({ 190.0f, 176.0f }, 32.0f, { 0.9f, 0.35f, 0.2f, 1.0f });
        guiRenderer.DrawCircle({ 270.0f, 176.0f }, 32.0f, { 0.15f, 0.15f, 0.18f, 1.0f }, 3.0f, { 0.4f, 0.9f, 0.5f, 1.0f });
        guiRenderer.DrawImage({ 48.0f, 208.0f }, { 112.0f, 272.0f }, checkerTexture);
        guiRenderer.DrawImage({ 128.0f, 208.0f }, { 192.0f, 272.0f }, checkerTexture, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f }, 32.0f);
        guiRenderer.Flush(screenProjection);

        if (font.IsLoaded())
        {
            textRenderer.DrawText(font, glyphCache, "Hello, Sloth!", { 32.0f, 64.0f }, 18.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, screenProjection);
        }

        engine.EndFrame();
        window.OnUpdate();
    }

    game.Shutdown();
    engine.Shutdown();

    return 0;
}
