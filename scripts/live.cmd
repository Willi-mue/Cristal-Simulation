@echo off
rem Build with the live viewer and open it. Extra arguments are passed to the simulation,
rem e.g.  scripts\live.cmd --mode radial --stick 0.3
setlocal
cd /d "%~dp0.."

cmake --preset gui || exit /b 1
cmake --build --preset gui || exit /b 1

build\gui\bin\cristal.exe --live %*
