@echo off
rem Build, simulate and render a video. Extra arguments are passed to the simulation,
rem e.g.  scripts\run.cmd --seed 42 --particles 10000
setlocal
cd /d "%~dp0.."

cmake --preset release || exit /b 1
cmake --build --preset release || exit /b 1

build\release\bin\cristal.exe %* || exit /b 1

python scripts\make_video.py || exit /b 1
