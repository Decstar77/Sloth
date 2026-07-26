@echo off
rem Launches two SandboxTower client windows tiled side by side across a
rem 1920x1080 screen (960x1080 each) for local multiplayer testing. The
rem first instance auto-spawns the local dev server (see SandboxTower's
rem main.cpp); the second detects it via SlothTowerServerMutex and just
rem connects - see the 1s delay below, which gives the first instance time
rem to win that race before the second one checks.

setlocal

set EXE=%~dp0bin\Debug-windows-x86_64\SandboxTower\SandboxTower.exe

if not exist "%EXE%" (
    echo SandboxTower.exe not found at "%EXE%" - build the Debug configuration first.
    exit /b 1
)

start "Tower Client 1" "%EXE%" --width 854 --height 480  --x 100 --y 400

rem ~1s delay. Not "timeout /t 1", which refuses to run without a real
rem console attached (fails under some launchers/CI); ping against
rem loopback is the standard batch-file workaround.
ping -n 2 127.0.0.1 >nul

start "Tower Client 2" "%EXE%" --width 854 --height 480 --x 960 --y 400

endlocal
