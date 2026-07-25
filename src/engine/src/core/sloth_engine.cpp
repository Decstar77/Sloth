#include "sloth_engine.h"

namespace sloth {
    Engine& Engine::Get() {
        static Engine instance;
        return instance;
    }

    void Engine::InitCommon() {
        SL_ASSERT_MSG( initialized == false, "Engine has already been initialized" );
        initialized = true;

        permanentArena.Init( PermanentArenaSize );
        frameArena.Init( FrameArenaSize );
    }

    void Engine::Init( const WindowProps& windowProps ) {
        InitCommon();

        window = std::make_unique<Window>( windowProps );
        input = std::make_unique<Input>( window->GetNativeWindow() );

        DebugRenderer::Get().Init();
    }

    void Engine::InitHeadless() {
        InitCommon();
        headless = true;
    }

    void Engine::Shutdown() {
        if ( headless == false ) {
            DebugRenderer::Get().Shutdown();
        }

        input.reset();
        window.reset();

        frameArena.Shutdown();
        permanentArena.Shutdown();
    }

    void Engine::EndFrame() {
        if ( input ) {
            input->Update();
        }
        frameArena.Reset();
    }

} // namespace sloth
