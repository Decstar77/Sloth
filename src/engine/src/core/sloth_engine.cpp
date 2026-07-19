#include "sloth_engine.h"

namespace sloth {
    Engine& Engine::Get() {
        static Engine instance;
        return instance;
    }

    void Engine::Init( const WindowProps& windowProps ) {
        SL_ASSERT_MSG( window == nullptr, "Engine has already been initialized" );

        permanentArena.Init( PermanentArenaSize );
        frameArena.Init( FrameArenaSize );

        window = std::make_unique<Window>( windowProps );
        input = std::make_unique<Input>( window->GetNativeWindow() );

        DebugRenderer::Get().Init();
    }

    void Engine::Shutdown() {
        DebugRenderer::Get().Shutdown();

        input.reset();
        window.reset();

        frameArena.Shutdown();
        permanentArena.Shutdown();
    }

    void Engine::EndFrame() {
        input->Update();
        frameArena.Reset();
    }

} // namespace sloth
