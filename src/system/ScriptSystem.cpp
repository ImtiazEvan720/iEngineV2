#include "system/ScriptSystem.h"

#include "Entity.h"
#include "components/AnimationComponent.h"
#include "components/CollisionComponent.h"
#include "components/ScriptComponent.h"
#include "components/TransformComponent.h"
#include "math/Vector2F.h"
#include "misc/Level.h"
#include "misc/LevelManager.h"
#include "system/PhysicsSystem.h"
#include "system/Renderer.h"
#include "system/VirtualInputSystem.h"

#ifdef IENGINE_EMBED_LUA_SCRIPTS
#include "EmbeddedScripts.h"
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <utility>
#include <vector>

#ifdef IENGINE_EMBED_LUA_SCRIPTS
namespace {
std::string getEmbeddedScriptKey(const std::string& scriptPath) {
    std::filesystem::path path(scriptPath);
    std::string genericPath = path.generic_string();

    const std::string relativePrefix = "Assets/Scripts/";
    if (genericPath.rfind(relativePrefix, 0) == 0) {
        return genericPath.substr(relativePrefix.size());
    }

    const std::string absoluteMarker = "/Assets/Scripts/";
    const std::size_t markerPosition = genericPath.find(absoluteMarker);
    if (markerPosition != std::string::npos) {
        return genericPath.substr(markerPosition + absoluteMarker.size());
    }

    return path.filename().generic_string();
}
}
#endif

namespace {
Entity* findEntityByName(const std::string& entityName) {
    for (Entity& entity : Level::getCurrentLevel().getEntities()) {
        if (!entity.isDestroyed() && entity.getName() == entityName) {
            return &entity;
        }
    }

    return nullptr;
}

Entity* findEntityByTag(const std::string& tag) {
    for (Entity& entity : Level::getCurrentLevel().getEntities()) {
        if (!entity.isDestroyed() && entity.getTag() == tag) {
            return &entity;
        }
    }

    return nullptr;
}

Entity* findEntityByGuid(const std::string& guid) {
    return Level::getCurrentLevel().getEntityByGuid(guid);
}

sol::object makeScriptValueObject(sol::state& lua, const ScriptValue& value) {
    switch (value.type) {
        case ScriptValueType::Int:
            return sol::make_object(lua, value.intValue);
        case ScriptValueType::Float:
            return sol::make_object(lua, value.floatValue);
        case ScriptValueType::Bool:
            return sol::make_object(lua, value.boolValue);
        case ScriptValueType::Vector2:
            return sol::make_object(lua, value.vector2Value);
        case ScriptValueType::Entity: {
            Entity* entity = findEntityByGuid(value.stringValue);
            return entity == nullptr
                ? sol::make_object(lua, sol::nil)
                : sol::make_object(lua, entity);
        }
        case ScriptValueType::Prefab:
        case ScriptValueType::String:
        default:
            return sol::make_object(lua, value.stringValue);
    }
}

sol::table makeRaycastResultTable(sol::state& lua, const PhysicsRaycastHit& hit) {
    sol::table result = lua.create_table();
    result["hit"] = hit.hit;
    result["point"] = hit.point;
    result["normal"] = hit.normal;
    result["fraction"] = hit.fraction;
    result["nodeVisits"] = hit.nodeVisits;
    result["leafVisits"] = hit.leafVisits;

    if (hit.collider != nullptr) {
        Entity* hitEntity = hit.collider->getEntity();
        result["colliderName"] = hit.collider->getName();
        result["collider"] = hit.collider;
        result["entity"] = hitEntity;
        result["entityName"] = hitEntity == nullptr ? "" : hitEntity->getName();
        result["tag"] = hitEntity == nullptr ? "" : hitEntity->getTag();
    } else {
        result["colliderName"] = "";
        result["collider"] = sol::nil;
        result["entity"] = sol::nil;
        result["entityName"] = "";
        result["tag"] = "";
    }

    return result;
}

std::uint8_t toRenderColorChannel(int value) {
    return static_cast<std::uint8_t>(std::clamp(value, 0, 255));
}
}

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
    refreshScriptPropertyTables(script, component);

#ifdef IENGINE_EMBED_LUA_SCRIPTS
    const std::string scriptKey = getEmbeddedScriptKey(component.getScriptPath());
    const char* embeddedSource = EmbeddedScripts::get(scriptKey);
    if (embeddedSource == nullptr) {
        std::cerr << "Embedded Lua script not found: " << scriptKey
                  << " from " << component.getScriptPath() << std::endl;
        return false;
    }

    sol::protected_function_result loadResult = lua.script(
        embeddedSource,
        script.environment,
        sol::script_pass_on_error
    );
    const std::string loadContext = "loading embedded " + scriptKey;
#else
    sol::protected_function_result loadResult = lua.script_file(
        component.getScriptPath(),
        script.environment,
        sol::script_pass_on_error
    );
    const std::string loadContext = "loading " + component.getScriptPath();
#endif

    if (!loadResult.valid()) {
        return reportScriptError(loadContext, loadResult);
    }

    script.onStart = script.environment.get<sol::protected_function>("onStart");
    script.onUpdate = script.environment.get<sol::protected_function>("onUpdate");

    auto [iterator, inserted] = scripts.emplace(&component, std::move(script));
    (void)inserted;

#ifdef IENGINE_EMBED_LUA_SCRIPTS
    std::cout << "Loaded embedded Lua script: " << scriptKey << std::endl;
#else
    std::cout << "Loaded Lua script: " << component.getScriptPath() << std::endl;
#endif
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

bool ScriptSystem::getEntityScriptFunction(
    Entity& entity,
    const std::string& functionName,
    sol::protected_function& function
) {
    if (!entity.isEnabled()) {
        std::cerr << "[Lua] Script target entity is disabled: " << entity.getName() << std::endl;
        return false;
    }

    ScriptComponent* scriptComponent = entity.getComponent<ScriptComponent>();
    if (scriptComponent == nullptr) {
        std::cerr << "[Lua] Script target has no ScriptComponent: " << entity.getName() << std::endl;
        return false;
    }

    const auto scriptIterator = scripts.find(scriptComponent);
    if (scriptIterator == scripts.end()) {
        std::cerr << "[Lua] Script target is not loaded: " << entity.getName() << std::endl;
        return false;
    }

    refreshScriptPropertyTables(scriptIterator->second, *scriptComponent);

    function = scriptIterator->second.environment.get<sol::protected_function>(functionName);
    if (!function.valid()) {
        std::cerr << "[Lua] Script function not found: "
                  << entity.getName() << "." << functionName << std::endl;
        return false;
    }

    return true;
}

void ScriptSystem::unloadScript(ScriptComponent& component) {
    scripts.erase(&component);
}

void ScriptSystem::refreshScriptPropertyTables(ScriptInstance& script, const ScriptComponent& component) {
    sol::table refs = lua.create_table();
    sol::table props = lua.create_table();

    for (const ScriptProperty& property : component.getProperties()) {
        if (property.name.empty()) {
            continue;
        }

        switch (property.type) {
            case ScriptPropertyType::Entity: {
                Entity* referencedEntity = component.getEntityReference(property.name);
                refs[property.name] = referencedEntity == nullptr
                    ? sol::make_object(lua, sol::nil)
                    : sol::make_object(lua, referencedEntity);
                break;
            }
            case ScriptPropertyType::Int:
                props[property.name] = property.intValue;
                break;
            case ScriptPropertyType::Float:
                props[property.name] = property.floatValue;
                break;
            case ScriptPropertyType::Bool:
                props[property.name] = property.boolValue;
                break;
            case ScriptPropertyType::Vector2:
                props[property.name] = property.vector2Value;
                break;
            case ScriptPropertyType::Array: {
                sol::table values = lua.create_table();
                int luaIndex = 1;
                for (const ScriptValue& value : property.arrayValue) {
                    values[luaIndex] = makeScriptValueObject(lua, value);
                    ++luaIndex;
                }

                props[property.name] = values;
                break;
            }
            case ScriptPropertyType::Map: {
                sol::table values = lua.create_table();
                for (const ScriptMapEntry& entry : property.mapValue) {
                    if (!entry.key.empty()) {
                        values[entry.key] = makeScriptValueObject(lua, entry.value);
                    }
                }

                props[property.name] = values;
                break;
            }
            case ScriptPropertyType::Prefab:
            case ScriptPropertyType::String:
            default:
                props[property.name] = property.stringValue;
                break;
        }
    }

    script.environment["Refs"] = refs;
    script.environment["Props"] = props;
}

bool ScriptSystem::callEntityScriptFunction(Entity& entity, const std::string& functionName) {
    sol::protected_function function;
    if (!getEntityScriptFunction(entity, functionName, function)) {
        return false;
    }

    sol::protected_function_result result = function();
    if (!result.valid()) {
        return reportScriptError(entity.getName() + "." + functionName, result);
    }

    return true;
}

bool ScriptSystem::callEntityScriptFunction(
    Entity& entity,
    const std::string& functionName,
    float argument
) {
    sol::protected_function function;
    if (!getEntityScriptFunction(entity, functionName, function)) {
        return false;
    }

    sol::protected_function_result result = function(argument);
    if (!result.valid()) {
        return reportScriptError(entity.getName() + "." + functionName, result);
    }

    return true;
}

bool ScriptSystem::callEntityScriptFunction(
    Entity& entity,
    const std::string& functionName,
    Entity& argument
) {
    sol::protected_function function;
    if (!getEntityScriptFunction(entity, functionName, function)) {
        return false;
    }

    sol::protected_function_result result = function(argument);
    if (!result.valid()) {
        return reportScriptError(entity.getName() + "." + functionName, result);
    }

    return true;
}

float ScriptSystem::callEntityScriptFloatFunction(
    Entity& entity,
    const std::string& functionName,
    float fallback
) {
    sol::protected_function function;
    if (!getEntityScriptFunction(entity, functionName, function)) {
        return fallback;
    }

    sol::protected_function_result result = function(entity);
    if (!result.valid()) {
        reportScriptError(entity.getName() + "." + functionName, result);
        return fallback;
    }

    sol::optional<float> value = result.get<sol::optional<float>>();
    if (!value.has_value()) {
        std::cerr << "[Lua] Script function did not return a number: "
                  << entity.getName() << "." << functionName << std::endl;
        return fallback;
    }

    return value.value();
}

bool ScriptSystem::callEntityScriptBoolFunction(
    Entity& entity,
    const std::string& functionName,
    bool fallback
) {
    sol::protected_function function;
    if (!getEntityScriptFunction(entity, functionName, function)) {
        return fallback;
    }

    sol::protected_function_result result = function(entity);
    if (!result.valid()) {
        reportScriptError(entity.getName() + "." + functionName, result);
        return fallback;
    }

    sol::optional<bool> value = result.get<sol::optional<bool>>();
    if (!value.has_value()) {
        std::cerr << "[Lua] Script function did not return a boolean: "
                  << entity.getName() << "." << functionName << std::endl;
        return fallback;
    }

    return value.value();
}

bool ScriptSystem::callEntityScriptFunctionWithSelf(
    Entity& entity,
    const std::string& functionName
) {
    sol::protected_function function;
    if (!getEntityScriptFunction(entity, functionName, function)) {
        return false;
    }

    sol::protected_function_result result = function(entity);
    if (!result.valid()) {
        return reportScriptError(entity.getName() + "." + functionName, result);
    }

    return true;
}

bool ScriptSystem::callEntityScriptFunctionWithSelf(
    Entity& entity,
    const std::string& functionName,
    Entity& argument
) {
    sol::protected_function function;
    if (!getEntityScriptFunction(entity, functionName, function)) {
        return false;
    }

    sol::protected_function_result result = function(entity, argument);
    if (!result.valid()) {
        return reportScriptError(entity.getName() + "." + functionName, result);
    }

    return true;
}

bool ScriptSystem::callEntityScriptEventFunction(Entity& entity, const std::string& eventType) {
    sol::table eventData = lua.create_table();
    return callEntityScriptEventFunction(entity, eventType, eventData);
}

bool ScriptSystem::callEntityScriptEventFunction(
    Entity& entity,
    const std::string& eventType,
    sol::table eventData
) {
    sol::protected_function function;
    if (!getEntityScriptFunction(entity, "onEvent", function)) {
        return false;
    }

    eventData["type"] = eventType;

    sol::protected_function_result result = function(entity, eventData);
    if (!result.valid()) {
        return reportScriptError(entity.getName() + ".onEvent", result);
    }

    sol::optional<bool> handled = result.get<sol::optional<bool>>();
    return !handled.has_value() || handled.value();
}

bool ScriptSystem::callEntityScriptFunction(
    Entity& entity,
    const std::string& functionName,
    Entity& entityArgument,
    float floatArgument
) {
    sol::protected_function function;
    if (!getEntityScriptFunction(entity, functionName, function)) {
        return false;
    }

    sol::protected_function_result result = function(entityArgument, floatArgument);
    if (!result.valid()) {
        return reportScriptError(entity.getName() + "." + functionName, result);
    }

    return true;
}

bool ScriptSystem::callEntityScriptFunction(
    Entity& entity,
    const std::string& functionName,
    const Vector2F& position,
    float rotation
) {
    sol::protected_function function;
    if (!getEntityScriptFunction(entity, functionName, function)) {
        return false;
    }

    sol::protected_function_result result = function(position, rotation);
    if (!result.valid()) {
        return reportScriptError(entity.getName() + "." + functionName, result);
    }

    return true;
}

bool ScriptSystem::callEntityScriptFunction(
    Entity& entity,
    const std::string& functionName,
    const Vector2F& position,
    float rotation,
    Entity& argument
) {
    sol::protected_function function;
    if (!getEntityScriptFunction(entity, functionName, function)) {
        return false;
    }

    sol::protected_function_result result = function(position, rotation, argument);
    if (!result.valid()) {
        return reportScriptError(entity.getName() + "." + functionName, result);
    }

    return true;
}

bool ScriptSystem::callEntityCollisionFunction(
    Entity& entity,
    const std::string& functionName,
    CollisionComponent& self,
    CollisionComponent& other,
    const Vector2F& normal,
    const Vector2F& contactPoint
) {
    sol::protected_function function;
    if (!getEntityScriptFunction(entity, functionName, function)) {
        return false;
    }

    Entity* otherEntity = other.getEntity();
    sol::protected_function_result result = function(
        entity,
        otherEntity == nullptr ? sol::make_object(lua, sol::nil) : sol::make_object(lua, otherEntity),
        &self,
        &other,
        normal,
        contactPoint
    );
    if (!result.valid()) {
        return reportScriptError(entity.getName() + "." + functionName, result);
    }

    return true;
}

bool ScriptSystem::tryCallEntityCollisionFunction(
    Entity& entity,
    const std::string& functionName,
    CollisionComponent& self,
    CollisionComponent& other,
    const Vector2F& normal,
    const Vector2F& contactPoint
) {
    if (!entity.isEnabled() || entity.isDestroyed()) {
        return false;
    }

    ScriptComponent* scriptComponent = entity.getComponent<ScriptComponent>();
    if (scriptComponent == nullptr) {
        return false;
    }

    const auto scriptIterator = scripts.find(scriptComponent);
    if (scriptIterator == scripts.end()) {
        return false;
    }

    refreshScriptPropertyTables(scriptIterator->second, *scriptComponent);

    sol::protected_function function =
        scriptIterator->second.environment.get<sol::protected_function>(functionName);
    if (!function.valid()) {
        return false;
    }

    Entity* otherEntity = other.getEntity();
    sol::protected_function_result result = function(
        entity,
        otherEntity == nullptr ? sol::make_object(lua, sol::nil) : sol::make_object(lua, otherEntity),
        &self,
        &other,
        normal,
        contactPoint
    );
    if (!result.valid()) {
        return reportScriptError(entity.getName() + "." + functionName, result);
    }

    return true;
}

bool ScriptSystem::callEntitySensorFunction(
    Entity& entity,
    const std::string& functionName,
    CollisionComponent& self,
    CollisionComponent& other
) {
    sol::protected_function function;
    if (!getEntityScriptFunction(entity, functionName, function)) {
        return false;
    }

    Entity* otherEntity = other.getEntity();
    sol::protected_function_result result = function(
        entity,
        otherEntity == nullptr ? sol::make_object(lua, sol::nil) : sol::make_object(lua, otherEntity),
        &self,
        &other
    );
    if (!result.valid()) {
        return reportScriptError(entity.getName() + "." + functionName, result);
    }

    return true;
}

bool ScriptSystem::tryCallEntitySensorFunction(
    Entity& entity,
    const std::string& functionName,
    CollisionComponent& self,
    CollisionComponent& other
) {
    if (!entity.isEnabled() || entity.isDestroyed()) {
        return false;
    }

    ScriptComponent* scriptComponent = entity.getComponent<ScriptComponent>();
    if (scriptComponent == nullptr) {
        return false;
    }

    const auto scriptIterator = scripts.find(scriptComponent);
    if (scriptIterator == scripts.end()) {
        return false;
    }

    refreshScriptPropertyTables(scriptIterator->second, *scriptComponent);

    sol::protected_function function =
        scriptIterator->second.environment.get<sol::protected_function>(functionName);
    if (!function.valid()) {
        return false;
    }

    Entity* otherEntity = other.getEntity();
    sol::protected_function_result result = function(
        entity,
        otherEntity == nullptr ? sol::make_object(lua, sol::nil) : sol::make_object(lua, otherEntity),
        &self,
        &other
    );
    if (!result.valid()) {
        return reportScriptError(entity.getName() + "." + functionName, result);
    }

    return true;
}

bool ScriptSystem::callEntityAnimationFinishedFunction(Entity& entity, const std::string& functionName) {
    if (!entity.isEnabled() || entity.isDestroyed()) {
        return false;
    }

    ScriptComponent* scriptComponent = entity.getComponent<ScriptComponent>();
    if (scriptComponent == nullptr) {
        return false;
    }

    const auto scriptIterator = scripts.find(scriptComponent);
    if (scriptIterator == scripts.end()) {
        return false;
    }

    refreshScriptPropertyTables(scriptIterator->second, *scriptComponent);

    sol::protected_function function =
        scriptIterator->second.environment.get<sol::protected_function>(functionName);
    if (!function.valid()) {
        return false;
    }

    sol::protected_function_result result = function(entity);
    if (!result.valid()) {
        return reportScriptError(entity.getName() + "." + functionName, result);
    }
    
    return true;
}
bool ScriptSystem::requestLevelLoad(const std::string& levelName) {
    if (levelName.empty()) {
        std::cerr << "[Lua] Cannot request level load with an empty level name." << std::endl;
        return false;
    }

    pendingLevelLoadName = levelName;
    std::cout << "[Lua] Requested level load: " << levelName << std::endl;
    return true;
}

bool ScriptSystem::consumePendingLevelLoad(std::string& levelName) {
    if (pendingLevelLoadName.empty()) {
        levelName.clear();
        return false;
    }

    levelName = pendingLevelLoadName;
    pendingLevelLoadName.clear();
    return true;
}

void ScriptSystem::bindEngineTypes() {
    lua["engineLog"] = [](const std::string& message) {
        std::cout << "[Lua] " << message << std::endl;
    };

    sol::table engineTable = lua.create_table();
    engineTable["log"] = [](const std::string& message) {
        std::cout << "[Lua] " << message << std::endl;
    };
    engineTable["loadLevel"] = [](const std::string& levelName) {
        return ScriptSystem::getInstance().requestLevelLoad(levelName);
    };
    engineTable["isEntityInViewport"] = [](Entity& entity) {
        return Renderer::getInstance().isEntityInViewport(entity);
    };
    engineTable["isWorldPointInViewport"] = [](const Vector2F& worldPoint, float margin) {
        if (!std::isfinite(worldPoint.x) || !std::isfinite(worldPoint.y)) {
            return false;
        }

        if (!std::isfinite(margin) || margin < 0.0f) {
            margin = 0.0f;
        }

        const Renderer& renderer = Renderer::getInstance();
        const RenderRect viewport = renderer.getViewport();
        const RenderRect worldViewport = renderer.getCamera().getWorldViewport(viewport);
        if (worldViewport.width <= 0.0f || worldViewport.height <= 0.0f) {
            return false;
        }

        return worldPoint.x >= worldViewport.x + margin
            && worldPoint.x <= worldViewport.x + worldViewport.width - margin
            && worldPoint.y >= worldViewport.y + margin
            && worldPoint.y <= worldViewport.y + worldViewport.height - margin;
    };
    engineTable["raycast"] = [this](const Vector2F& start, const Vector2F& end) {
        const PhysicsRaycastHit hit = PhysicsSystem::getInstance().raycast(start, end);
        return makeRaycastResultTable(lua, hit);
    };
    engineTable["debugDrawPoint"] = sol::overload(
        [](const Vector2F& worldPosition) {
            Renderer::getInstance().debugDrawPoint(
                worldPosition,
                4.0f,
                RenderColor{255, 255, 0, 255}
            );
        },
        [](const Vector2F& worldPosition, float radius) {
            Renderer::getInstance().debugDrawPoint(
                worldPosition,
                radius,
                RenderColor{255, 255, 0, 255}
            );
        },
        [](const Vector2F& worldPosition, float radius, int r, int g, int b) {
            Renderer::getInstance().debugDrawPoint(
                worldPosition,
                radius,
                RenderColor{
                    toRenderColorChannel(r),
                    toRenderColorChannel(g),
                    toRenderColorChannel(b),
                    255
                }
            );
        },
        [](const Vector2F& worldPosition, float radius, int r, int g, int b, int a) {
            Renderer::getInstance().debugDrawPoint(
                worldPosition,
                radius,
                RenderColor{
                    toRenderColorChannel(r),
                    toRenderColorChannel(g),
                    toRenderColorChannel(b),
                    toRenderColorChannel(a)
                }
            );
        },
        [](const Vector2F& worldPosition, float radius, int r, int g, int b, int a, float lifetimeSeconds) {
            Renderer::getInstance().debugDrawPoint(
                worldPosition,
                radius,
                RenderColor{
                    toRenderColorChannel(r),
                    toRenderColorChannel(g),
                    toRenderColorChannel(b),
                    toRenderColorChannel(a)
                },
                lifetimeSeconds
            );
        }
    );
    engineTable["clearDebugDraw"] = []() {
        Renderer::getInstance().clearDebugDraw();
    };
    engineTable["findEntityByName"] = [](const std::string& entityName) {
        return findEntityByName(entityName);
    };
    engineTable["findEntityByTag"] = [](const std::string& tag) {
        return findEntityByTag(tag);
    };
    engineTable["findEntityByGuid"] = [](const std::string& guid) {
        return findEntityByGuid(guid);
    };
    engineTable["getActiveLevels"] = [this]() {
        sol::table levels = lua.create_table();
        LevelManager& levelManager = LevelManager::getInstance();
        levelManager.load();

        const std::vector<const LevelEntry*> activeLevels =
            levelManager.getActiveLevelEntries();

        int luaIndex = 1;
        for (const LevelEntry* entry : activeLevels) {
            if (entry != nullptr) {
                levels[luaIndex] = entry->fileName;
                ++luaIndex;
            }
        }

        return levels;
    };
    lua["Engine"] = engineTable;

    sol::table inputTable = lua.create_table();
    inputTable["isActionDown"] = [](const std::string& actionName) {
        return VirtualInputSystem::getInstance().isActionDown(inputActionFromString(actionName));
    };
    inputTable["wasActionPressed"] = [](const std::string& actionName) {
        return VirtualInputSystem::getInstance().wasActionPressed(inputActionFromString(actionName));
    };
    inputTable["wasActionReleased"] = [](const std::string& actionName) {
        return VirtualInputSystem::getInstance().wasActionReleased(inputActionFromString(actionName));
    };
    lua["Input"] = inputTable;

    lua["spawnPrefab"] = sol::overload(
        [](const std::string& prefabName, float x, float y) {
            return Entity::spawnPrefab(prefabName, x, y);
        },
        [](const std::string& prefabName, float x, float y, float rotation) {
            return Entity::spawnPrefab(prefabName, x, y, rotation);
        },
        [](const std::string& prefabName, const Vector2F& position) {
            return Entity::spawnPrefab(prefabName, position);
        },
        [](const std::string& prefabName, const Vector2F& position, float rotation) {
            return Entity::spawnPrefab(prefabName, position, rotation);
        }
    );

    lua.new_usertype<Vector2F>(
        "Vector2F",
        sol::constructors<Vector2F(float, float)>(),
        "x", &Vector2F::x,
        "y", &Vector2F::y
    );

    lua.new_usertype<TransformComponent>(
        "TransformComponent",
        "isEnabled", &TransformComponent::isEnabled,
        "setEnabled", &TransformComponent::setEnabled,
        "getPosition", [](const TransformComponent& transform) {
            return transform.getPosition();
        },
        "getWorldPosition", [](const TransformComponent& transform) {
            return transform.getWorldPosition();
        },
        "setPosition", [](TransformComponent& transform, const Vector2F& position) {
            transform.setPosition(position);
        },
        "getRotation", &TransformComponent::getRotation,
        "getWorldRotation", &TransformComponent::getWorldRotation,
        "setRotation", &TransformComponent::setRotation
    );

    lua.new_usertype<AnimationComponent>(
        "AnimationComponent",
        "isEnabled", &AnimationComponent::isEnabled,
        "setEnabled", &AnimationComponent::setEnabled,
        "play", &AnimationComponent::play,
        "pause", &AnimationComponent::pause,
        "reset", &AnimationComponent::reset,
        "isPlaying", &AnimationComponent::isPlaying,
        "isFinished", &AnimationComponent::isFinished,
        "getCurrentFrameIndex", &AnimationComponent::getCurrentFrameIndex,
        "setLooping", &AnimationComponent::setLooping
    );

    lua.new_usertype<CollisionComponent>(
        "CollisionComponent",
        "isEnabled", &CollisionComponent::isEnabled,
        "setEnabled", &CollisionComponent::setEnabled,
        "getName", &CollisionComponent::getName,
        "getWidth", &CollisionComponent::getWidth,
        "getHeight", &CollisionComponent::getHeight,
        "getRotation", &CollisionComponent::getRotation,
        "getWorldRotation", &CollisionComponent::getWorldRotation,
        "setRotation", &CollisionComponent::setRotation,
        "isSensor", &CollisionComponent::isSensor,
        "getLinearVelocity", &CollisionComponent::getLinearVelocity,
        "setLinearVelocity", &CollisionComponent::setLinearVelocity,
        "isFixedRotation", &CollisionComponent::isFixedRotation,
        "setFixedRotation", &CollisionComponent::setFixedRotation
    );

    lua.new_usertype<ScriptComponent>(
        "ScriptComponent",
        "isEnabled", &ScriptComponent::isEnabled,
        "setEnabled", &ScriptComponent::setEnabled,
        "getString", [](const ScriptComponent& component, const std::string& name, const std::string& fallback) {
            return component.getString(name, fallback);
        },
        "getPrefab", [](const ScriptComponent& component, const std::string& name, const std::string& fallback) {
            return component.getString(name, fallback);
        },
        "getEntity", [](const ScriptComponent& component, const std::string& name) {
            return component.getEntityReference(name);
        },
        "getInt", [](const ScriptComponent& component, const std::string& name, int fallback) {
            return component.getInt(name, fallback);
        },
        "getFloat", [](const ScriptComponent& component, const std::string& name, float fallback) {
            return component.getFloat(name, fallback);
        },
        "getBool", [](const ScriptComponent& component, const std::string& name, bool fallback) {
            return component.getBool(name, fallback);
        }
    );

    lua.new_usertype<Entity>(
        "Entity",
        "getId", &Entity::getId,
        "getGuid", &Entity::getGuid,
        "getName", &Entity::getName,
        "setName", &Entity::setName,
        "getTag", &Entity::getTag,
        "setTag", &Entity::setTag,
        "isEnabled", &Entity::isEnabled,
        "isDestroyed", &Entity::isDestroyed,
        "setEnabled", &Entity::setEnabled,
        "setPersistent", &Entity::setPersistent,
        "isPersistent", &Entity::isPersistent,
        "destroy", sol::overload(
            [](Entity& entity) {
                entity.destroy(true);
            },
            [](Entity& entity, bool destroyChildren) {
                entity.destroy(destroyChildren);
            }
        ),
        "setParent", sol::overload(
            [](Entity& entity, Entity* parent) {
                return entity.setParent(parent);
            },
            [](Entity& entity, Entity* parent, bool keepWorldTransform) {
                return entity.setParent(parent, keepWorldTransform);
            }
        ),
        "clearParent", sol::overload(
            [](Entity& entity) {
                entity.clearParent();
            },
            [](Entity& entity, bool keepWorldTransform) {
                entity.clearParent(keepWorldTransform);
            }
        ),
        "getTransform", [](Entity& entity) {
            return entity.getComponent<TransformComponent>();
        },
        "getAnimation", [](Entity& entity) {
            return entity.getComponent<AnimationComponent>();
        },
        "getCollision", [](Entity& entity) {
            return entity.getComponent<CollisionComponent>();
        },
        "isInViewport", [](Entity& entity) {
            return Renderer::getInstance().isEntityInViewport(entity);
        },
        "sendEvent", sol::overload(
            [](Entity& entity, const std::string& eventType) {
                return ScriptSystem::getInstance().callEntityScriptEventFunction(entity, eventType);
            },
            [](Entity& entity, const std::string& eventType, sol::table eventData) {
                return ScriptSystem::getInstance().callEntityScriptEventFunction(entity, eventType, eventData);
            }
        ),
        "getScript", [](Entity& entity) {
            return entity.getComponent<ScriptComponent>();
        },
        "spawnPrefab", sol::overload(
            [](const std::string& prefabName, float x, float y) {
                return Entity::spawnPrefab(prefabName, x, y);
            },
            [](const std::string& prefabName, float x, float y, float rotation) {
                return Entity::spawnPrefab(prefabName, x, y, rotation);
            },
            [](const std::string& prefabName, const Vector2F& position) {
                return Entity::spawnPrefab(prefabName, position);
            },
            [](const std::string& prefabName, const Vector2F& position, float rotation) {
                return Entity::spawnPrefab(prefabName, position, rotation);
            }
        )
    );
}

bool ScriptSystem::callOnStart(ScriptInstance& script, Entity& entity) {
    if (!script.onStart.valid()) {
        return true;
    }

    ScriptComponent* scriptComponent = entity.getComponent<ScriptComponent>();
    if (scriptComponent != nullptr) {
        refreshScriptPropertyTables(script, *scriptComponent);
    }

    sol::protected_function_result result = script.onStart(entity, scriptComponent);
    if (!result.valid()) {
        return reportScriptError(entity.getName() + ".onStart", result);
    }

    return true;
}

bool ScriptSystem::callOnUpdate(ScriptInstance& script, Entity& entity, float deltaTime) {
    if (!script.onUpdate.valid()) {
        return true;
    }

    ScriptComponent* scriptComponent = entity.getComponent<ScriptComponent>();
    if (scriptComponent != nullptr) {
        refreshScriptPropertyTables(script, *scriptComponent);
    }

    sol::protected_function_result result = script.onUpdate(entity, deltaTime, scriptComponent);
    if (!result.valid()) {
        return reportScriptError(entity.getName() + ".onUpdate", result);
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
