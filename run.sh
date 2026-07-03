#!/bin/bash
PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
"$PROJECT_ROOT/build/bin/iEngineV2" --data-root "$PROJECT_ROOT" --sfml "$@"
