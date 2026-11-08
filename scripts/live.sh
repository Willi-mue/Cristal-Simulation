#!/usr/bin/env sh
# Build with the live viewer and open it. Extra arguments are passed to the simulation,
# e.g.  sh scripts/live.sh --mode radial --stick 0.3
set -e
cd "$(dirname "$0")/.."

cmake --preset gui
cmake --build --preset gui

./build/gui/bin/cristal --live "$@"
