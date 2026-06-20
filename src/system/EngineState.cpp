#include "system/EngineState.h"

EngineState& EngineState::getInstance() {
    static EngineState instance;
    return instance;
}

EngineState::RuntimeMode EngineState::getRuntimeMode() const {
    return runtimeMode;
}

void EngineState::setRuntimeMode(RuntimeMode value) {
    runtimeMode = value;
}

bool EngineState::isPlaying() const {
    return runtimeMode == RuntimeMode::Play;
}

bool EngineState::isGamePaused() const {
    return runtimeMode != RuntimeMode::Play;
}

void EngineState::setGamePaused(bool value) {
    runtimeMode = value ? RuntimeMode::Pause : RuntimeMode::Play;
}
