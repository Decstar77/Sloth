@echo off
pushd %~dp0
call premake5.exe vs2026
popd
pause
