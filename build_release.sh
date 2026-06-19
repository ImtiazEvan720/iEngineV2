#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

cmake -S . -B build-release \
    -DCMAKE_BUILD_TYPE=Release \
    -DIENGINE_WITH_EDITOR=OFF

cmake --build build-release --config Release
