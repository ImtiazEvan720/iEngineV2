#!/bin/bash
cd "$(dirname "$0")/build/bin" || exit 1
./iEngineV2 --sdl "$@"
