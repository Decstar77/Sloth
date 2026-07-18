#include "sloth_input.h"

#include <cstring>

#include <GLFW/glfw3.h>

namespace sloth
{
    Input* Input::instance = nullptr;

    Input::Input(GLFWwindow* windowHandle)
        : windowHandle(windowHandle)
    {
        SL_ASSERT_MSG(instance == nullptr, "Only one Input instance may exist at a time");
        instance = this;

        glfwGetCursorPos(windowHandle, &mouseX, &mouseY);

        glfwSetKeyCallback(windowHandle, KeyCallback);
        glfwSetMouseButtonCallback(windowHandle, MouseButtonCallback);
        glfwSetCursorPosCallback(windowHandle, CursorPosCallback);
        glfwSetScrollCallback(windowHandle, ScrollCallback);
    }

    Input::~Input()
    {
        glfwSetKeyCallback(windowHandle, nullptr);
        glfwSetMouseButtonCallback(windowHandle, nullptr);
        glfwSetCursorPosCallback(windowHandle, nullptr);
        glfwSetScrollCallback(windowHandle, nullptr);

        instance = nullptr;
    }

    void Input::Update()
    {
        std::memcpy(keysDownPrevious, keysDown, sizeof(keysDown));
        std::memcpy(mouseButtonsDownPrevious, mouseButtonsDown, sizeof(mouseButtonsDown));

        mouseDeltaX = 0.0;
        mouseDeltaY = 0.0;
        scrollDeltaX = 0.0;
        scrollDeltaY = 0.0;
    }

    bool Input::IsKeyDown(Key key) const
    {
        return keysDown[static_cast<usize>(key)];
    }

    bool Input::IsKeyPressed(Key key) const
    {
        usize index = static_cast<usize>(key);
        return keysDown[index] && !keysDownPrevious[index];
    }

    bool Input::IsKeyReleased(Key key) const
    {
        usize index = static_cast<usize>(key);
        return !keysDown[index] && keysDownPrevious[index];
    }

    bool Input::IsMouseButtonDown(MouseButton button) const
    {
        return mouseButtonsDown[static_cast<usize>(button)];
    }

    bool Input::IsMouseButtonPressed(MouseButton button) const
    {
        usize index = static_cast<usize>(button);
        return mouseButtonsDown[index] && !mouseButtonsDownPrevious[index];
    }

    bool Input::IsMouseButtonReleased(MouseButton button) const
    {
        usize index = static_cast<usize>(button);
        return !mouseButtonsDown[index] && mouseButtonsDownPrevious[index];
    }

    void Input::KeyCallback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods)
    {
        SL_UNUSED(window);
        SL_UNUSED(scancode);
        SL_UNUSED(mods);

        if (key < 0 || static_cast<usize>(key) >= KeyCount || action == GLFW_REPEAT)
        {
            return;
        }

        instance->keysDown[static_cast<usize>(key)] = (action == GLFW_PRESS);
    }

    void Input::MouseButtonCallback(GLFWwindow* window, i32 button, i32 action, i32 mods)
    {
        SL_UNUSED(window);
        SL_UNUSED(mods);

        if (button < 0 || static_cast<usize>(button) >= MouseButtonCount)
        {
            return;
        }

        instance->mouseButtonsDown[static_cast<usize>(button)] = (action == GLFW_PRESS);
    }

    void Input::CursorPosCallback(GLFWwindow* window, f64 x, f64 y)
    {
        SL_UNUSED(window);

        instance->mouseDeltaX += x - instance->mouseX;
        instance->mouseDeltaY += y - instance->mouseY;
        instance->mouseX = x;
        instance->mouseY = y;
    }

    void Input::ScrollCallback(GLFWwindow* window, f64 xOffset, f64 yOffset)
    {
        SL_UNUSED(window);

        instance->scrollDeltaX += xOffset;
        instance->scrollDeltaY += yOffset;
    }

} // namespace sloth
