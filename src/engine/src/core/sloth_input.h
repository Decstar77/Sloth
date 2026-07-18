#pragma once

#include "core/sloth_defines.h"

struct GLFWwindow;

namespace sloth
{

    // Values match GLFW's key codes directly so callbacks can index storage
    // arrays with the raw code, no translation table required.
    enum class Key : i32
    {
        Space = 32,

        D0 = 48, D1, D2, D3, D4, D5, D6, D7, D8, D9,

        A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

        Escape = 256,
        Enter,
        Tab,
        Backspace,
        Insert,
        Delete,
        Right,
        Left,
        Down,
        Up,
        PageUp,
        PageDown,
        Home,
        End,

        CapsLock = 280,

        F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

        LeftShift = 340,
        LeftControl,
        LeftAlt,
        LeftSuper,
        RightShift,
        RightControl,
        RightAlt,
        RightSuper,
    };

    enum class MouseButton : i32
    {
        Left = 0,
        Right = 1,
        Middle = 2,
    };

    // Polls keyboard/mouse state for the single application window. Owned by
    // Engine; reached via Engine::Get().GetInput(). Registers GLFW callbacks
    // on construction; Update() must be called once per frame (Engine::EndFrame()
    // does this) to advance edge-triggered state and reset per-frame deltas
    // before the next Window::OnUpdate() pumps GLFW events.
    class Input
    {
    public:
        explicit Input(GLFWwindow* windowHandle);
        ~Input();

        SL_NON_COPYABLE(Input);
        SL_NON_MOVABLE(Input);

        void Update();

        bool IsKeyDown(Key key) const;
        bool IsKeyPressed(Key key) const;  // True only on the frame the key went down.
        bool IsKeyReleased(Key key) const; // True only on the frame the key went up.

        bool IsMouseButtonDown(MouseButton button) const;
        bool IsMouseButtonPressed(MouseButton button) const;
        bool IsMouseButtonReleased(MouseButton button) const;

        f64 GetMouseX() const { return mouseX; }
        f64 GetMouseY() const { return mouseY; }
        f64 GetMouseDeltaX() const { return mouseDeltaX; }
        f64 GetMouseDeltaY() const { return mouseDeltaY; }

        f64 GetScrollDeltaX() const { return scrollDeltaX; }
        f64 GetScrollDeltaY() const { return scrollDeltaY; }

    private:
        static constexpr usize KeyCount = 350;
        static constexpr usize MouseButtonCount = 8;

        static void KeyCallback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods);
        static void MouseButtonCallback(GLFWwindow* window, i32 button, i32 action, i32 mods);
        static void CursorPosCallback(GLFWwindow* window, f64 x, f64 y);
        static void ScrollCallback(GLFWwindow* window, f64 xOffset, f64 yOffset);

    private:
        // Sloth is single-engine/single-window; Input mirrors that by keeping
        // its own static self-pointer for GLFW callbacks rather than fighting
        // Window over the one GLFW user-pointer slot.
        static Input* instance;

        GLFWwindow* windowHandle = nullptr;

        bool keysDown[KeyCount] = {};
        bool keysDownPrevious[KeyCount] = {};

        bool mouseButtonsDown[MouseButtonCount] = {};
        bool mouseButtonsDownPrevious[MouseButtonCount] = {};

        f64 mouseX = 0.0;
        f64 mouseY = 0.0;
        f64 mouseDeltaX = 0.0;
        f64 mouseDeltaY = 0.0;

        f64 scrollDeltaX = 0.0;
        f64 scrollDeltaY = 0.0;
    };

} // namespace sloth
