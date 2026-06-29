#pragma once

#include <sol/sol.hpp>

#include <string>
#include <unordered_map>

class Entity;
class ScriptComponent;
class Vector2F;
class CollisionComponent;

class ScriptSystem {
public:
    static ScriptSystem& getInstance();

    void initialize();
    bool loadScript(ScriptComponent& component);
    void updateScript(ScriptComponent& component, float deltaTime);
    void unloadScript(ScriptComponent& component);
    bool callEntityScriptFunction(Entity& entity, const std::string& functionName);
    bool callEntityScriptFunction(Entity& entity, const std::string& functionName, float argument);
    bool callEntityScriptFunction(Entity& entity, const std::string& functionName, Entity& argument);
    bool callEntityScriptFunctionWithSelf(Entity& entity, const std::string& functionName);
    bool callEntityScriptFunctionWithSelf(Entity& entity, const std::string& functionName, Entity& argument);
    bool callEntityScriptFunction(Entity& entity, const std::string& functionName, Entity& entityArgument, float floatArgument);
    bool callEntityScriptFunction(Entity& entity, const std::string& functionName, const Vector2F& position, float rotation);
    bool callEntityScriptFunction(
        Entity& entity,
        const std::string& functionName,
        const Vector2F& position,
        float rotation,
        Entity& argument
    );
    bool callEntityCollisionFunction(
        Entity& entity,
        const std::string& functionName,
        CollisionComponent& self,
        CollisionComponent& other,
        const Vector2F& normal,
        const Vector2F& contactPoint
    );
    bool tryCallEntityCollisionFunction(
        Entity& entity,
        const std::string& functionName,
        CollisionComponent& self,
        CollisionComponent& other,
        const Vector2F& normal,
        const Vector2F& contactPoint
    );
    bool callEntitySensorFunction(
        Entity& entity,
        const std::string& functionName,
        CollisionComponent& self,
        CollisionComponent& other
    );
    bool tryCallEntitySensorFunction(
        Entity& entity,
        const std::string& functionName,
        CollisionComponent& self,
        CollisionComponent& other
    );
    bool requestLevelLoad(const std::string& levelName);
    bool consumePendingLevelLoad(std::string& levelName);
    float callEntityScriptFloatFunction(
        Entity& entity,
        const std::string& functionName,
        float fallback
    );

private:
    struct ScriptInstance {
        sol::environment environment;
        sol::protected_function onStart;
        sol::protected_function onUpdate;
    };

    ScriptSystem() = default;

    void bindEngineTypes();
    void refreshScriptPropertyTables(ScriptInstance& script, const ScriptComponent& component);
    bool getEntityScriptFunction(Entity& entity, const std::string& functionName, sol::protected_function& function);
    bool callOnStart(ScriptInstance& script, Entity& entity);
    bool callOnUpdate(ScriptInstance& script, Entity& entity, float deltaTime);
    bool reportScriptError(const std::string& context, const sol::protected_function_result& result);

    sol::state lua;
    std::unordered_map<ScriptComponent*, ScriptInstance> scripts;
    std::string pendingLevelLoadName;
    bool initialized = false;
};
