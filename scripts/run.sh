#!/usr/bin/env sh
# Build, simulate and render a video. Extra arguments are passed to the simulation,
# e.g.  sh scripts/run.sh --seed 42 --particles 10000
set -e
cd "$(dirname "$0")/.."

cmake --preset release
cmake --build --preset release

./build/release/bin/cristal "$@"

python3 scripts/make_video.py
