#include <core/sloth_engine.h>

#include <tower_server.h>

#include <atomic>
#include <chrono>
#include <thread>

#if defined( SLOTH_PLATFORM_WINDOWS )
    #include <Windows.h>
#endif

using namespace sloth;

static const f32 TargetFrameTime = 1.0f / 60.0f;

#if defined( SLOTH_PLATFORM_WINDOWS )
static std::atomic<bool> s_running = true;

static BOOL WINAPI ConsoleCtrlHandler( DWORD signal ) {
    if ( signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT || signal == CTRL_SHUTDOWN_EVENT ) {
        s_running = false;
        return TRUE;
    }
    return FALSE;
}
#endif

int main() {
#if defined( SLOTH_PLATFORM_WINDOWS )
    SetConsoleCtrlHandler( ConsoleCtrlHandler, TRUE );

    // Held for the lifetime of the process (Windows releases it automatically
    // on exit, clean or not) so SandboxTower can detect whether a server is
    // already running before spawning one of its own.
    HANDLE serverMutex = CreateMutexA( nullptr, FALSE, tower::ServerMutexName );
    SL_UNUSED( serverMutex );
#endif

    Engine & engine = Engine::Get();
    engine.InitHeadless();

    tower::TowerServer server;
    server.Init( tower::DefaultServerPort );

    auto lastFrameTime = std::chrono::steady_clock::now();

#if defined( SLOTH_PLATFORM_WINDOWS )
    while ( s_running ) {
#else
    while ( true ) {
#endif
        auto currentFrameTime = std::chrono::steady_clock::now();
        f32 deltaTime = std::chrono::duration<f32>( currentFrameTime - lastFrameTime ).count();
        lastFrameTime = currentFrameTime;

        server.Update( deltaTime );
        engine.EndFrame();

        f32 elapsed = std::chrono::duration<f32>( std::chrono::steady_clock::now() - currentFrameTime ).count();
        if ( elapsed < TargetFrameTime ) {
            std::this_thread::sleep_for( std::chrono::duration<f32>( TargetFrameTime - elapsed ) );
        }
    }

    server.Shutdown();
    engine.Shutdown();

    return 0;
}
