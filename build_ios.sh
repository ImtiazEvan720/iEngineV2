#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

BUILD_DIR="${BUILD_DIR:-build-ios}"
SDK="${SDK:-iphonesimulator}"
CONFIG="${CONFIG:-Debug}"
ARCHS="${ARCHS:-arm64}"
DEPLOYMENT_TARGET="${DEPLOYMENT_TARGET:-14.0}"

if [ -z "${EMBED_LUA_SCRIPTS:-}" ]; then
    if [ "$CONFIG" = "Release" ]; then
        EMBED_LUA_SCRIPTS="ON"
    else
        EMBED_LUA_SCRIPTS="OFF"
    fi
fi

if ! command -v xcodebuild >/dev/null 2>&1; then
    echo "xcodebuild was not found. Install Xcode first."
    exit 1
fi

CODE_SIGNING_ALLOWED="${CODE_SIGNING_ALLOWED:-NO}"
if [ "$SDK" = "iphoneos" ] && [ "${CODE_SIGNING_ALLOWED}" = "NO" ]; then
    echo "Building for a real iOS device usually requires code signing."
    echo "Set CODE_SIGNING_ALLOWED=YES plus your Xcode signing settings if needed."
fi

cmake -S . -B "$BUILD_DIR" -G Xcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_SYSROOT="$SDK" \
    -DCMAKE_OSX_ARCHITECTURES="$ARCHS" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOYMENT_TARGET" \
    -DIENGINE_EMBED_LUA_SCRIPTS="$EMBED_LUA_SCRIPTS" \
    -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED="$CODE_SIGNING_ALLOWED"

cmake --build "$BUILD_DIR" --config "$CONFIG"
