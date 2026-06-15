#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

export JAVA_HOME="${IENGINE_JAVA_HOME:-/Library/Java/JavaVirtualMachines/jdk-21.jdk/Contents/Home}"
export ANDROID_HOME="${ANDROID_HOME:-$HOME/Library/Android/sdk}"
export ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-$ANDROID_HOME}"
export PATH="$JAVA_HOME/bin:$ANDROID_HOME/platform-tools:$ANDROID_HOME/emulator:$ANDROID_HOME/cmdline-tools/latest/bin:$PATH"

if [ ! -d "$ANDROID_HOME" ]; then
    echo "Android SDK not found: $ANDROID_HOME"
    exit 1
fi

if [ ! -x "$JAVA_HOME/bin/java" ]; then
    echo "Java 17+ not found at JAVA_HOME: $JAVA_HOME"
    exit 1
fi

(
    cd android
    ./gradlew assembleDebug
)

APK="android/app/build/outputs/apk/debug/app-debug.apk"
echo "Built Android APK: $APK"

if [ "${INSTALL:-0}" = "1" ]; then
    adb install -r "$APK"
fi

if [ "${LAUNCH:-0}" = "1" ]; then
    adb shell monkey -p com.imtiazevan.ienginev2 1
fi
