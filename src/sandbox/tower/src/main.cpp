#include <core/sloth_engine.h>

#include <tower_game.h>
#include <tower_server.h>

// Must come before glfw3.h - Windows.h's minwindef.h unconditionally
// #defines APIENTRY, whereas GLFW only defines it if not already defined.
// Including this second would trigger a macro-redefinition warning.
#if defined( SLOTH_PLATFORM_WINDOWS )
    #include <Windows.h>
#endif

#include <GLFW/glfw3.h>

#include <string>

using namespace sloth;

#if defined( SLOTH_PLATFORM_WINDOWS )
namespace {
    PROCESS_INFORMATION g_spawnedServer = {};
    bool                g_spawnedServerRunning = false;

    bool IsServerAlreadyRunning() {
        HANDLE mutex = OpenMutexA( SYNCHRONIZE, FALSE, tower::ServerMutexName );
        if ( mutex != nullptr ) {
            CloseHandle( mutex );
            return true;
        }
        return false;
    }

    // Dev convenience only: brings up a local SandboxTowerServer.exe so
    // pressing play on the client is enough to get a working local match,
    // without needing to start the server separately by hand.
    void SpawnLocalServer() {
        char exePath[MAX_PATH];
        GetModuleFileNameA( nullptr, exePath, MAX_PATH );

        // Each premake project builds into its own bin/<config>/<ProjectName>/
        // subfolder, so SandboxTowerServer.exe is a sibling directory of ours
        // rather than sitting next to SandboxTower.exe - go up one more level
        // (.../bin/<config>/SandboxTower/main.exe -> .../bin/<config>/).
        std::string exeDir = exePath;
        usize lastSlash = exeDir.find_last_of( "\\/" );
        std::string binConfigDir = exeDir.substr( 0, lastSlash );
        usize parentSlash = binConfigDir.find_last_of( "\\/" );
        std::string serverPath = binConfigDir.substr( 0, parentSlash + 1 ) + "SandboxTowerServer\\SandboxTowerServer.exe";

        STARTUPINFOA startupInfo = { sizeof( STARTUPINFOA ) };
        PROCESS_INFORMATION processInfo = {};

        // CREATE_NEW_PROCESS_GROUP (without a new console) lets us target
        // just this child with GenerateConsoleCtrlEvent later, while its
        // log output still lands in the same console window as ours.
        BOOL ok = CreateProcessA(
            serverPath.c_str(), nullptr, nullptr, nullptr, FALSE,
            CREATE_NEW_PROCESS_GROUP, nullptr, nullptr, &startupInfo, &processInfo );

        if ( ok ) {
            g_spawnedServer = processInfo;
            g_spawnedServerRunning = true;
            SL_LOG_INFO( "Spawned local TowerServer (pid %lu)", processInfo.dwProcessId );
        } else {
            SL_LOG_ERROR( "Failed to spawn local TowerServer at '%s' (error %lu)", serverPath.c_str(), GetLastError() );
        }
    }

    void StopLocalServerIfSpawned() {
        if ( !g_spawnedServerRunning ) {
            return;
        }

        GenerateConsoleCtrlEvent( CTRL_C_EVENT, g_spawnedServer.dwProcessId );

        if ( WaitForSingleObject( g_spawnedServer.hProcess, 2000 ) != WAIT_OBJECT_0 ) {
            TerminateProcess( g_spawnedServer.hProcess, 1 );
        }

        CloseHandle( g_spawnedServer.hProcess );
        CloseHandle( g_spawnedServer.hThread );
        g_spawnedServerRunning = false;
    }
} // namespace
#endif

int main() {
#if defined( SLOTH_PLATFORM_WINDOWS )
    if ( !IsServerAlreadyRunning() ) {
        SpawnLocalServer();
    }
#endif

    WindowProps props;
    props.Title = "Sloth Engine - Tower";
    props.Width = 1280;
    props.Height = 720;

    Engine & engine = Engine::Get();
    engine.Init( props );

    Window & window = engine.GetWindow();

    tower::TowerGame game;
    game.Init();

    f64 lastFrameTime = glfwGetTime();

    while ( !window.ShouldClose() ) {
        if ( engine.GetInput().IsKeyPressed( Key::Escape ) == true ) {
            window.SetShouldClose( true );
        }

        f64 currentFrameTime = glfwGetTime();
        f32 deltaTime = static_cast<f32>( currentFrameTime - lastFrameTime );
        lastFrameTime = currentFrameTime;

        game.Update( deltaTime );
        game.Render();

        engine.EndFrame();
        window.OnUpdate();
    }

    game.Shutdown();
    engine.Shutdown();

#if defined( SLOTH_PLATFORM_WINDOWS )
    StopLocalServerIfSpawned();
#endif

    return 0;
}
