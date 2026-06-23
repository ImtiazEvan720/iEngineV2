#include "system/ScriptSystem.h"

#include "Entity.h"
#include "components/AnimationComponent.h"
#include "components/ScriptComponent.h"
#include "components/TransformComponent.h"
#include "math/Vector2F.h"
#include "misc/LevelManager.h"
#include "system/Renderer.h"
#include "system/VirtualInputSystem.h"

#ifdef IENGINE_EMBED_LUA_SCRIPTS
#include "EmbeddedScripts.h"
#endif

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

void ScriptSystem::unloadScript(ScriptComponent& component) {
    scripts.erase(&component);
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
        "getPosition", [](const TransformComponent& transform) {
            return transform.getPosition();
        },
        "setPosition", [](TransformComponent& transform, const Vector2F& position) {
            transform.setPosition(position);
        },
        "getRotation", &TransformComponent::getRotation,
        "setRotation", &TransformComponent::setRotation
    );

    lua.new_usertype<AnimationComponent>(
        "AnimationComponent",
        "play", &AnimationComponent::play,
        "pause", &AnimationComponent::pause,
        "reset", &AnimationComponent::reset,
        "isPlaying", &AnimationComponent::isPlaying,
        "isFinished", &AnimationComponent::isFinished,
        "setLooping", &AnimationComponent::setLooping
    );

    lua.new_usertype<ScriptComponent>(
        "ScriptComponent",
        "getString", [](const ScriptComponent& component, const std::string& name, const std::string& fallback) {
            return component.getString(name, fallback);
        },
        "getPrefab", [](const ScriptComponent& component, const std::string& name, const std::string& fallback) {
            return component.getString(name, fallback);
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
        "getName", &Entity::getName,
        "setName", &Entity::setName,
        "getTag", &Entity::getTag,
        "setTag", &Entity::setTag,
        "isEnabled", &Entity::isEnabled,
        "setEnabled", &Entity::setEnabled,
        "setPersistent", &Entity::setPersistent,
        "isPersistent", &Entity::isPersistent,
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
        "isInViewport", [](Entity& entity) {
            return Renderer::getInstance().isEntityInViewport(entity);
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
    sol::protected_function_result result = script.onStart(entity, scriptComponent);
    if (!result.valid()) {
        return reportScriptError("onStart", result);
    }

    return true;
}

bool ScriptSystem::callOnUpdate(ScriptInstance& script, Entity& entity, float deltaTime) {
    if (!script.onUpdate.valid()) {
        return true;
    }

    ScriptComponent* scriptComponent = entity.getComponent<ScriptComponent>();
    sol::protected_function_result result = script.onUpdate(entity, deltaTime, scriptComponent);
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
