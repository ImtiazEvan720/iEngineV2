#!/bin/bash
cd "$(dirname "$0")/build-release/bin" || exit 1
./iEngineV2 --sfml "$@"
