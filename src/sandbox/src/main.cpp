#include <core/sloth_engine.h>
#include <font/sloth_font.h>
#include <gui/sloth_gui_context.h>
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
    font.Load("../../assets/fonts/roboto/Roboto-Regular.ttf");
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

    GuiContext guiContext;

    dust::DustGame game;
    game.Init();

    f64 lastFrameTime = glfwGetTime();

    while (!window.ShouldClose())
    {
        f64 currentFrameTime = glfwGetTime();
        f32 deltaTime = static_cast<f32>(currentFrameTime - lastFrameTime);
        lastFrameTime = currentFrameTime;

        guiContext.NewFrame(engine.GetInput());

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

        // Hand-rolled clickable rect exercising GuiContext directly (no
        // Button() widget wrapper exists yet) - the classic hot/active
        // state machine, plus GuiStorage to persist a click count per-ID
        // across frames.
        {
            glm::vec2 buttonMin{ 32.0f, 320.0f };
            glm::vec2 buttonMax{ 232.0f, 368.0f };
            GuiId buttonId = guiContext.GetId("DemoButton");

            bool hovered = GuiContext::IsPointInRect(guiContext.GetMousePos(), buttonMin, buttonMax);
            if (hovered)
            {
                guiContext.SetHot(buttonId);
            }

            if (guiContext.IsHot(buttonId) && guiContext.IsMousePressed())
            {
                guiContext.SetActive(buttonId);
                guiContext.SetFocused(buttonId);
            }

            bool clicked = false;
            if (guiContext.IsActive(buttonId) && guiContext.IsMouseReleased())
            {
                clicked = guiContext.IsHot(buttonId);
                guiContext.ClearActive();
            }

            i32& clickCount = guiContext.GetStorage().GetOrAddInt(buttonId);
            if (clicked)
            {
                ++clickCount;
            }

            glm::vec4 buttonColor = guiContext.IsActive(buttonId) ? glm::vec4{ 0.15f, 0.4f, 0.7f, 1.0f }
                                   : guiContext.IsHot(buttonId)   ? glm::vec4{ 0.3f, 0.65f, 1.0f, 1.0f }
                                                                   : glm::vec4{ 0.2f, 0.55f, 0.9f, 1.0f };
            guiRenderer.DrawRect(buttonMin, buttonMax, buttonColor, 8.0f);

            guiRenderer.Flush(screenProjection);

            if (font.IsLoaded())
            {
                LargeString buttonLabel;
                buttonLabel.Format("Clicked: %d", clickCount);
                textRenderer.DrawText(font, glyphCache, buttonLabel.View(), buttonMin + glm::vec2(12.0f, 30.0f), 16.0f,
                                       { 1.0f, 1.0f, 1.0f, 1.0f }, screenProjection);
            }
        }

        if (font.IsLoaded())
        {
            textRenderer.DrawText(font, glyphCache, "Hello, Sloth! i am engine clicked", { 32.0f, 64.0f }, 28.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, screenProjection);
        }

        // Clip-rect / scissor demo: a small "viewport" panel showing a list
        // of rows that overflow both above and below it. Rows outside the
        // viewport should be cut off cleanly at its edge instead of
        // spilling onto the rest of the screen, and the mouse shouldn't be
        // able to hover a row's clipped-off portion.
        {
            glm::vec2 viewportMin{ 360.0f, 96.0f };
            glm::vec2 viewportMax{ 560.0f, 280.0f };

            guiRenderer.DrawRect(viewportMin, viewportMax, { 0.1f, 0.1f, 0.13f, 1.0f }, 8.0f);

            guiContext.PushClipRect(viewportMin, viewportMax);
            guiRenderer.PushClipRect(viewportMin, viewportMax);

            constexpr f32 rowHeight = 36.0f;
            constexpr f32 scrollOffset = 40.0f; // Pretend the list has been scrolled down a bit.
            for (i32 row = 0; row < 8; ++row)
            {
                f32 y = viewportMin.y + 8.0f + static_cast<f32>(row) * rowHeight - scrollOffset;
                glm::vec2 rowMin{ viewportMin.x + 8.0f, y };
                glm::vec2 rowMax{ viewportMax.x - 8.0f, y + rowHeight - 6.0f };

                bool hovered = GuiContext::IsPointInRect(guiContext.GetMousePos(), rowMin, rowMax) && guiContext.IsPointVisible(guiContext.GetMousePos());
                glm::vec4 rowColor = hovered ? glm::vec4{ 0.35f, 0.35f, 0.42f, 1.0f } : glm::vec4{ 0.22f, 0.22f, 0.28f, 1.0f };
                guiRenderer.DrawRect(rowMin, rowMax, rowColor, 4.0f);
            }

            guiRenderer.PopClipRect();
            guiContext.PopClipRect();

            guiRenderer.Flush(screenProjection);
        }

        guiContext.EndFrame();
        engine.EndFrame();
        window.OnUpdate();
    }

    game.Shutdown();
    engine.Shutdown();

    return 0;
}
