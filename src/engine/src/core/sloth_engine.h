#pragma once

#include "core/sloth_arena.h"
#include "core/sloth_defines.h"
#include "core/sloth_input.h"
#include "core/sloth_window.h"

#include <memory>

namespace sloth {
    // Owns all engine-wide global state (the window, memory arenas, etc).
    // Sloth is single-engine only; use Engine::Get() to access it from
    // anywhere once it has been initialized.
    class Engine {
    public:
        static constexpr usize PermanentArenaSize = 64ull * 1024 * 1024; // 64 MB, lives for the entire program.
        static constexpr usize FrameArenaSize = 64ull * 1024 * 1024;     // 64 MB, cleared at the end of every frame.

        SL_NON_COPYABLE( Engine );
        SL_NON_MOVABLE( Engine );

        static Engine&  Get();

        void            Init( const WindowProps& windowProps = WindowProps() );
        void            Shutdown();

        // Clears the frame arena and advances Input's edge-triggered state
        // (pressed/released, deltas). Call once per frame, after the frame's
        // work has been submitted but BEFORE Window::OnUpdate() — Input's
        // previous-state snapshot must happen before GLFW events are polled,
        // otherwise "pressed this frame" edges are overwritten before any
        // game code observes them.
        void            EndFrame();

        Window&         GetWindow() { return *window; }
        Input&          GetInput() { return *input; }
        Arena&          GetPermanentArena() { return permanentArena; }
        Arena&          GetFrameArena() { return frameArena; }

    private:
                        Engine() = default;
                        ~Engine() = default;

    private:
        std::unique_ptr<Window> window;
        std::unique_ptr<Input>  input;
        Arena                   permanentArena;
        Arena                   frameArena;
    };

} // namespace sloth
