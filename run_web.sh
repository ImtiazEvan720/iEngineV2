#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

PORT="${PORT:-8000}"
WEB_DIR="build-web/bin"

if [ ! -d "$WEB_DIR" ]; then
    echo "Web build directory not found: $WEB_DIR"
    echo "Build it first with:"
    echo "  source /Users/imtiazevan/emsdk/emsdk_env.sh"
    echo "  emcmake cmake -S . -B build-web"
    echo "  cmake --build build-web"
    exit 1
fi

echo "Serving web build at http://localhost:${PORT}/iEngineV2.html"
python3 -m http.server "$PORT" -d "$WEB_DIR"
