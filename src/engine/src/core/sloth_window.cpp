#include "sloth_window.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace sloth
{
    Window::Window(const WindowProps& props)
        : title(props.Title), width(props.Width), height(props.Height), vsync(props.VSync)
    {
        if (!glfwInit())
        {
            SL_LOG_FATAL("Failed to initialize GLFW");
            std::exit(EXIT_FAILURE);
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        windowHandle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!windowHandle)
        {
            SL_LOG_FATAL("Failed to create GLFW window");
            glfwTerminate();
            std::exit(EXIT_FAILURE);
        }

        glfwGetWindowPos(windowHandle, &posX, &posY);
        windowedWidth = width;
        windowedHeight = height;
        windowedPosX = posX;
        windowedPosY = posY;

        glfwSetWindowUserPointer(windowHandle, this);
        glfwSetFramebufferSizeCallback(windowHandle, FramebufferSizeCallback);
        glfwSetWindowPosCallback(windowHandle, WindowPosCallback);

        glfwMakeContextCurrent(windowHandle);

        if (!gladLoadGL(glfwGetProcAddress))
        {
            SL_LOG_FATAL("Failed to initialize GLAD");
            std::exit(EXIT_FAILURE);
        }

        SetVSync(vsync);
    }

    Window::~Window()
    {
        glfwDestroyWindow(windowHandle);
        glfwTerminate();
    }

    void Window::OnUpdate()
    {
        glfwSwapBuffers(windowHandle);
        glfwPollEvents();
    }

    bool Window::ShouldClose() const
    {
        return glfwWindowShouldClose(windowHandle) != 0;
    }

    void Window::SetShouldClose(bool shouldClose)
    {
        glfwSetWindowShouldClose(windowHandle, shouldClose ? GLFW_TRUE : GLFW_FALSE);
    }

    void Window::GetPosition(i32& outX, i32& outY) const
    {
        outX = posX;
        outY = posY;
    }

    void Window::SetPosition(i32 x, i32 y)
    {
        glfwSetWindowPos(windowHandle, x, y);
        posX = x;
        posY = y;
    }

    void Window::Resize(i32 newWidth, i32 newHeight)
    {
        glfwSetWindowSize(windowHandle, newWidth, newHeight);
        width = newWidth;
        height = newHeight;
    }

    void Window::SetTitle(const std::string& newTitle)
    {
        title = newTitle;
        glfwSetWindowTitle(windowHandle, title.c_str());
    }

    void Window::SetVSync(bool enabled)
    {
        vsync = enabled;
        glfwSwapInterval(vsync ? 1 : 0);
    }

    void Window::SetCursorMode(CursorMode mode)
    {
        cursorMode = mode;

        int glfwMode = GLFW_CURSOR_NORMAL;
        switch (mode)
        {
            case CursorMode::Normal:   glfwMode = GLFW_CURSOR_NORMAL;   break;
            case CursorMode::Hidden:   glfwMode = GLFW_CURSOR_HIDDEN;   break;
            case CursorMode::Disabled: glfwMode = GLFW_CURSOR_DISABLED; break;
        }

        glfwSetInputMode(windowHandle, GLFW_CURSOR, glfwMode);
    }

    void Window::GetCursorPosition(f64& outX, f64& outY) const
    {
        glfwGetCursorPos(windowHandle, &outX, &outY);
    }

    void Window::SetCursorPosition(f64 x, f64 y)
    {
        glfwSetCursorPos(windowHandle, x, y);
    }

    void Window::SetFullscreen(bool enabled)
    {
        if (enabled == fullscreen)
        {
            return;
        }

        fullscreen = enabled;

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (!monitor)
        {
            SL_LOG_WARN("SetFullscreen: no primary monitor available");
            return;
        }

        if (fullscreen)
        {
            // Cache current windowed geometry so we can restore it later.
            windowedWidth = width;
            windowedHeight = height;
            windowedPosX = posX;
            windowedPosY = posY;

            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(windowHandle, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            width = mode->width;
            height = mode->height;
        }
        else
        {
            glfwSetWindowMonitor(windowHandle, nullptr, windowedPosX, windowedPosY, windowedWidth, windowedHeight, 0);
            width = windowedWidth;
            height = windowedHeight;
            posX = windowedPosX;
            posY = windowedPosY;
        }

        SetVSync(vsync);
    }

    bool Window::IsFocused() const
    {
        return glfwGetWindowAttrib(windowHandle, GLFW_FOCUSED) != 0;
    }

    bool Window::IsMinimized() const
    {
        return glfwGetWindowAttrib(windowHandle, GLFW_ICONIFIED) != 0;
    }

    void Window::Focus()
    {
        glfwFocusWindow(windowHandle);
    }

    void Window::GetMonitorSize(i32& outWidth, i32& outHeight) const
    {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (!monitor)
        {
            outWidth = 0;
            outHeight = 0;
            return;
        }

        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        outWidth = mode->width;
        outHeight = mode->height;
    }

    i32 Window::GetMonitorRefreshRate() const
    {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (!monitor)
        {
            return 0;
        }

        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        return mode->refreshRate;
    }

    void Window::FramebufferSizeCallback(GLFWwindow* window, int newWidth, int newHeight)
    {
        Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        self->width = newWidth;
        self->height = newHeight;

        glViewport(0, 0, newWidth, newHeight);
    }

    void Window::WindowPosCallback(GLFWwindow* window, int x, int y)
    {
        Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        self->posX = x;
        self->posY = y;
    }
} // namespace Sloth
