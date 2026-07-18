#pragma once

#include "core/sloth_defines.h"

#include <string>

struct GLFWwindow;

namespace sloth
{

    enum class CursorMode : u8
    {
        Normal = 0, // Cursor is visible and behaves normally.
        Hidden,     // Cursor is invisible when over the window, but not constrained.
        Disabled,   // Cursor is hidden and locked to the window, providing unbounded movement (e.g. for FPS-style look).
    };

    struct WindowProps
    {
        std::string Title = "Sloth Engine";
        i32 Width = 1280;
        i32 Height = 720;
        bool VSync = true;
    };

    // Represents the single application window and its GLFW/OpenGL context.
    // Sloth is single-window only; the instance lives on Engine and is
    // reached via Engine::Get().GetWindow().
    class Window
    {
    public:
        explicit Window(const WindowProps& props = WindowProps());
        ~Window();

        SL_NON_COPYABLE(Window);
        SL_NON_MOVABLE(Window);

        void OnUpdate();
        bool ShouldClose() const;
        void SetShouldClose(bool shouldClose);

        // Size / position
        i32 GetWidth() const { return width; }
        i32 GetHeight() const { return height; }
        f32 GetAspectRatio() const { return height != 0 ? static_cast<f32>(width) / static_cast<f32>(height) : 0.0f; }

        void GetPosition(i32& outX, i32& outY) const;
        void SetPosition(i32 x, i32 y);

        void Resize(i32 newWidth, i32 newHeight);

        // Title
        const std::string& GetTitle() const { return title; }
        void SetTitle(const std::string& newTitle);

        // VSync
        bool IsVSync() const { return vsync; }
        void SetVSync(bool enabled);

        // Cursor
        CursorMode GetCursorMode() const { return cursorMode; }
        void SetCursorMode(CursorMode mode);

        void GetCursorPosition(f64& outX, f64& outY) const;
        void SetCursorPosition(f64 x, f64 y);

        // Fullscreen
        bool IsFullscreen() const { return fullscreen; }
        void SetFullscreen(bool enabled);
        void ToggleFullscreen() { SetFullscreen(!fullscreen); }

        // Focus / minimize
        bool IsFocused() const;
        bool IsMinimized() const;
        void Focus();

        // Screen / monitor queries. These describe the monitor the window
        // currently lives on, not the window itself.
        void GetMonitorSize(i32& outWidth, i32& outHeight) const;
        i32 GetMonitorRefreshRate() const;

        GLFWwindow* GetNativeWindow() const { return windowHandle; }

    private:
        static void FramebufferSizeCallback(GLFWwindow* window, int newWidth, int newHeight);
        static void WindowPosCallback(GLFWwindow* window, int x, int y);

    private:
        GLFWwindow* windowHandle = nullptr;

        std::string title;
        i32 width = 0;
        i32 height = 0;
        i32 posX = 0;
        i32 posY = 0;
        bool vsync = true;
        bool fullscreen = false;
        CursorMode cursorMode = CursorMode::Normal;

        // Cached windowed-mode geometry, used to restore the window when
        // leaving fullscreen.
        i32 windowedWidth = 0;
        i32 windowedHeight = 0;
        i32 windowedPosX = 0;
        i32 windowedPosY = 0;
    };

} // namespace Sloth
