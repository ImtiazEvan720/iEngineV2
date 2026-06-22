#pragma once

#include <sol/sol.hpp>

#include <string>
#include <unordered_map>

class Entity;
class ScriptComponent;

class ScriptSystem {
public:
    static ScriptSystem& getInstance();

    void initialize();
    bool loadScript(ScriptComponent& component);
    void updateScript(ScriptComponent& component, float deltaTime);
    void unloadScript(ScriptComponent& component);

private:
    struct ScriptInstance {
        sol::environment environment;
        sol::protected_function onStart;
        sol::protected_function onUpdate;
    };

    ScriptSystem() = default;

    void bindEngineTypes();
    bool callOnStart(ScriptInstance& script, Entity& entity);
    bool callOnUpdate(ScriptInstance& script, Entity& entity, float deltaTime);
    bool reportScriptError(const std::string& context, const sol::protected_function_result& result);

    sol::state lua;
    std::unordered_map<ScriptComponent*, ScriptInstance> scripts;
    bool initialized = false;
};
