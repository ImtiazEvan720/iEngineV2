#include "components/ScriptComponent.h"

#include "system/ScriptSystem.h"

#include <utility>

ScriptComponent::ScriptComponent(std::string scriptPath)
    : scriptPath(std::move(scriptPath)) {}

const std::string& ScriptComponent::getScriptPath() const {
    return scriptPath;
}

bool ScriptComponent::isLoaded() const {
    return loaded;
}

void ScriptComponent::onStart() {
    loaded = ScriptSystem::getInstance().loadScript(*this);
}

void ScriptComponent::onUpdate(float deltaTime) {
    if (!loaded) {
        return;
    }

    ScriptSystem::getInstance().updateScript(*this, deltaTime);
}

void ScriptComponent::onDestroy() {
    ScriptSystem::getInstance().unloadScript(*this);
    loaded = false;
}
