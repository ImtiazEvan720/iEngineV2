#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

EMSDK_ENV="/Users/imtiazevan/emsdk/emsdk_env.sh"

if [ ! -f "$EMSDK_ENV" ]; then
    echo "Emscripten environment script not found: $EMSDK_ENV"
    echo "Install emsdk first, then try again."
    exit 1
fi

source "$EMSDK_ENV"

emcmake cmake -S . -B build-web \
    -DCMAKE_BUILD_TYPE=Release \
    -DIENGINE_WITH_EDITOR=OFF \
    -DIENGINE_EMBED_LUA_SCRIPTS=ON
cmake --build build-web
