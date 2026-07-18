#include "sloth_engine.h"

namespace sloth
{
    Engine& Engine::Get()
    {
        static Engine instance;
        return instance;
    }

    void Engine::Init(const WindowProps& windowProps)
    {
        SL_ASSERT_MSG(window == nullptr, "Engine has already been initialized");

        permanentArena.Init(PermanentArenaSize);
        frameArena.Init(FrameArenaSize);

        window = std::make_unique<Window>(windowProps);
    }

    void Engine::Shutdown()
    {
        window.reset();

        frameArena.Shutdown();
        permanentArena.Shutdown();
    }

    void Engine::EndFrame()
    {
        frameArena.Reset();
    }

} // namespace sloth
