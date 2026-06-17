#include "system/ScriptSystem.h"

#include "Entity.h"
#include "components/ScriptComponent.h"
#include "components/TransformComponent.h"
#include "math/Vector2F.h"

#include <iostream>
#include <utility>

ScriptSystem& ScriptSystem::getInstance() {
    static ScriptSystem instance;
    return instance;
}

void ScriptSystem::initialize() {
    if (initialized) {
        return;
    }

    lua.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::string,
        sol::lib::table
    );

    bindEngineTypes();
    initialized = true;
}

bool ScriptSystem::loadScript(ScriptComponent& component) {
    initialize();
    unloadScript(component);

    Entity* entity = component.getEntity();
    if (entity == nullptr) {
        std::cerr << "ScriptComponent has no entity for script: "
                  << component.getScriptPath() << std::endl;
        return false;
    }

    ScriptInstance script;
    script.environment = sol::environment(lua, sol::create, lua.globals());

    sol::protected_function_result loadResult = lua.script_file(
        component.getScriptPath(),
        script.environment,
        sol::script_pass_on_error
    );

    if (!loadResult.valid()) {
        return reportScriptError("loading " + component.getScriptPath(), loadResult);
    }

    script.onStart = script.environment.get<sol::protected_function>("onStart");
    script.onUpdate = script.environment.get<sol::protected_function>("onUpdate");

    auto [iterator, inserted] = scripts.emplace(&component, std::move(script));
    (void)inserted;

    std::cout << "Loaded Lua script: " << component.getScriptPath() << std::endl;
    return callOnStart(iterator->second, *entity);
}

void ScriptSystem::updateScript(ScriptComponent& component, float deltaTime) {
    auto iterator = scripts.find(&component);
    if (iterator == scripts.end()) {
        return;
    }

    Entity* entity = component.getEntity();
    if (entity == nullptr) {
        return;
    }

    callOnUpdate(iterator->second, *entity, deltaTime);
}

void ScriptSystem::unloadScript(ScriptComponent& component) {
    scripts.erase(&component);
}

void ScriptSystem::bindEngineTypes() {
    lua["engineLog"] = [](const std::string& message) {
        std::cout << "[Lua] " << message << std::endl;
    };

    lua.new_usertype<Vector2F>(
        "Vector2F",
        sol::constructors<Vector2F(float, float)>(),
        "x", &Vector2F::x,
        "y", &Vector2F::y
    );

    lua.new_usertype<TransformComponent>(
        "TransformComponent",
        "getPosition", [](const TransformComponent& transform) {
            return transform.getPosition();
        },
        "setPosition", [](TransformComponent& transform, const Vector2F& position) {
            transform.setPosition(position);
        },
        "getRotation", &TransformComponent::getRotation,
        "setRotation", &TransformComponent::setRotation
    );

    lua.new_usertype<Entity>(
        "Entity",
        "getId", &Entity::getId,
        "getName", &Entity::getName,
        "getTag", &Entity::getTag,
        "getTransform", [](Entity& entity) {
            return entity.getComponent<TransformComponent>();
        }
    );
}

bool ScriptSystem::callOnStart(ScriptInstance& script, Entity& entity) {
    if (!script.onStart.valid()) {
        return true;
    }

    sol::protected_function_result result = script.onStart(entity);
    if (!result.valid()) {
        return reportScriptError("onStart", result);
    }

    return true;
}

bool ScriptSystem::callOnUpdate(ScriptInstance& script, Entity& entity, float deltaTime) {
    if (!script.onUpdate.valid()) {
        return true;
    }

    sol::protected_function_result result = script.onUpdate(entity, deltaTime);
    if (!result.valid()) {
        return reportScriptError("onUpdate", result);
    }

    return true;
}

bool ScriptSystem::reportScriptError(
    const std::string& context,
    const sol::protected_function_result& result
) {
    sol::error error = result;
    std::cerr << "Lua error during " << context << ": " << error.what() << std::endl;
    return false;
}
