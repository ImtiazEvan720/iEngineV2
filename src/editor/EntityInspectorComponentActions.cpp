#include "editor/EntityInspectorPanel.h"

#include "Entity.h"
#include "components/CollisionComponent.h"
#include "components/AnimationComponent.h"
#include "components/PlayerController.h"
#include "components/ScriptComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "editor/EditorCollectionViews.h"
#include "editor/ScriptPropertyParser.h"
#include "game/Brick.h"
#include "game/Bullet.h"
#include "math/Vector2F.h"
#include "system/InputSystem.h"
#include "system/ProjectManager.h"

#include "imgui.h"

#include <algorithm>
#include <filesystem>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {
template <typename TComponent, typename... Args>
bool addComponentIfMissing(Entity& entity, Args&&... args) {
    if (entity.getComponent<TComponent>() != nullptr) {
        return false;
    }

    entity.addComponent<TComponent>(std::forward<Args>(args)...);
    return true;
}

template <typename TComponent, typename... Args>
void ensureComponent(Entity& entity, Args&&... args) {
    if (entity.getComponent<TComponent>() == nullptr) {
        entity.addComponent<TComponent>(std::forward<Args>(args)...);
    }
}

template <typename TComponent>
void removeDependencyIfPresent(Entity& entity) {
    if (entity.getComponent<TComponent>() != nullptr) {
        entity.removeComponent<TComponent>();
    }
}

bool addScriptComponentWithDefaults(Entity& entity, const std::string& scriptPath) {
    if (entity.getComponent<ScriptComponent>() != nullptr || scriptPath.empty()) {
        return false;
    }

    entity.addComponent<ScriptComponent>(scriptPath, ScriptPropertyParser::makeDefaultScriptProperties(scriptPath));
    return true;
}

struct ComponentAddEntry {
    const char* name;
    bool needsScriptPath = false;
    std::function<bool(Entity&, const std::string&)> add;
};

struct ComponentRemoveEntry {
    const char* name;
    std::function<bool(Entity&)> remove;
};

std::vector<std::string> findLuaScripts() {
    std::vector<std::string> scripts;
    const std::filesystem::path scriptsPath =
        ProjectManager::getInstance().getAssetsPath() / "Scripts";

    if (!std::filesystem::exists(scriptsPath)) {
        return scripts;
    }

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(scriptsPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".lua") {
                scripts.push_back(entry.path().generic_string());
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        scripts.clear();
    }

    std::sort(scripts.begin(), scripts.end());
    return scripts;
}

std::string makeScriptListLabel(const std::string& scriptPath) {
    const std::filesystem::path path(scriptPath);
    const std::filesystem::path scriptsPath =
        ProjectManager::getInstance().getAssetsPath() / "Scripts";

    std::error_code errorCode;
    const std::filesystem::path relativePath =
        std::filesystem::relative(path, scriptsPath, errorCode);
    if (!errorCode && !relativePath.empty()) {
        return relativePath.generic_string();
    }

    return path.filename().string();
}

const std::vector<ComponentAddEntry>& getComponentAddRegistry() {
    static const std::vector<ComponentAddEntry> registry = {
        {
            "TransformComponent",
            false,
            [](Entity& entity, const std::string& scriptPath) {
                (void)scriptPath;
                return addComponentIfMissing<TransformComponent>(
                    entity,
                    Vector2F(0.0f, 0.0f),
                    0.0f
                );
            }
        },
        {
            "CollisionComponent",
            false,
            [](Entity& entity, const std::string& scriptPath) {
                (void)scriptPath;
                ensureComponent<TransformComponent>(entity, Vector2F(0.0f, 0.0f), 0.0f);
                return addComponentIfMissing<CollisionComponent>(
                    entity,
                    64.0f,
                    64.0f,
                    CollisionComponent::BodyType::Static,
                    false,
                    "Collider"
                );
            }
        },
        {
            "ScriptComponent",
            true,
            [](Entity& entity, const std::string& scriptPath) {
                return addScriptComponentWithDefaults(entity, scriptPath);
            }
        },
        {
            "PlayerController",
            false,
            [](Entity& entity, const std::string& scriptPath) {
                (void)scriptPath;
                ensureComponent<TransformComponent>(entity, Vector2F(0.0f, 0.0f), 0.0f);
                return addComponentIfMissing<ScriptComponent>(
                    entity,
                    "Assets/Scripts/player_controller.lua"
                );
            }
        },
        {
            "Brick",
            false,
            [](Entity& entity, const std::string& scriptPath) {
                (void)scriptPath;
                ensureComponent<TransformComponent>(entity, Vector2F(0.0f, 0.0f), 0.0f);
                ensureComponent<CollisionComponent>(
                    entity,
                    64.0f,
                    64.0f,
                    CollisionComponent::BodyType::Static,
                    false,
                    "Brick"
                );
                return addComponentIfMissing<Brick>(entity);
            }
        },
        {
            "Bullet",
            false,
            [](Entity& entity, const std::string& scriptPath) {
                (void)scriptPath;
                ensureComponent<TransformComponent>(entity, Vector2F(0.0f, 0.0f), 0.0f);
                ensureComponent<CollisionComponent>(
                    entity,
                    16.0f,
                    32.0f,
                    CollisionComponent::BodyType::Dynamic,
                    true,
                    "Bullet"
                );
                return addComponentIfMissing<Bullet>(entity);
            }
        }
    };

    return registry;
}

std::vector<ComponentRemoveEntry> getComponentRemoveEntries(Entity& entity) {
    std::vector<ComponentRemoveEntry> entries;

    if (entity.getComponent<TransformComponent>() != nullptr) {
        entries.push_back({
            "TransformComponent",
            [](Entity& target) {
                removeDependencyIfPresent<PlayerController>(target);
                removeDependencyIfPresent<Bullet>(target);
                return target.removeComponent<TransformComponent>();
            }
        });
    }

    if (entity.getComponent<SpriteComponent>() != nullptr) {
        entries.push_back({
            "SpriteComponent",
            [](Entity& target) {
                return target.removeComponent<SpriteComponent>();
            }
        });
    }

    if (entity.getComponent<AnimationComponent>() != nullptr) {
        entries.push_back({
            "AnimationComponent",
            [](Entity& target) {
                removeDependencyIfPresent<PlayerController>(target);
                return target.removeComponent<AnimationComponent>();
            }
        });
    }

    if (entity.getComponent<CollisionComponent>() != nullptr) {
        entries.push_back({
            "CollisionComponent",
            [](Entity& target) {
                removeDependencyIfPresent<Brick>(target);
                removeDependencyIfPresent<Bullet>(target);
                return target.removeComponent<CollisionComponent>();
            }
        });
    }

    if (entity.getComponent<ScriptComponent>() != nullptr) {
        entries.push_back({
            "ScriptComponent",
            [](Entity& target) {
                return target.removeComponent<ScriptComponent>();
            }
        });
    }

    if (entity.getComponent<PlayerController>() != nullptr) {
        entries.push_back({
            "PlayerController",
            [](Entity& target) {
                return target.removeComponent<PlayerController>();
            }
        });
    }

    if (entity.getComponent<Brick>() != nullptr) {
        entries.push_back({
            "Brick",
            [](Entity& target) {
                return target.removeComponent<Brick>();
            }
        });
    }

    if (entity.getComponent<Bullet>() != nullptr) {
        entries.push_back({
            "Bullet",
            [](Entity& target) {
                return target.removeComponent<Bullet>();
            }
        });
    }

    return entries;
}

bool removeComponentByName(Entity& entity, const std::string& componentName) {
    const std::vector<ComponentRemoveEntry> entries = getComponentRemoveEntries(entity);

    for (const ComponentRemoveEntry& entry : entries) {
        if (componentName == entry.name) {
            return entry.remove(entity);
        }
    }

    return false;
}
}

void EntityInspectorPanel::drawAddComponentCombo(Entity& entity, std::string& statusMessage) {
    const std::vector<ComponentAddEntry>& registry = getComponentAddRegistry();
    if (registry.empty()) {
        return;
    }

    if (selectedAddComponentIndex < 0 ||
        selectedAddComponentIndex >= static_cast<int>(registry.size())) {
        selectedAddComponentIndex = 0;
    }

    std::vector<std::string> luaScripts;
    std::string selectedScriptPath;

    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::BeginCombo("Component", registry[static_cast<std::size_t>(selectedAddComponentIndex)].name)) {
        std::vector<ListViewItem> componentItems;
        componentItems.reserve(registry.size());
        for (std::size_t index = 0; index < registry.size(); ++index) {
            const ComponentAddEntry& entry = registry[index];
            componentItems.push_back(ListViewItem{
                entry.name,
                entry.name,
                entry.needsScriptPath ? "Requires a Lua script selection." : ""
            });
        }

        ListViewOptions options;
        options.height = 180.0f;
        options.border = false;
        options.emptyText = "No components.";
        EditorCollectionViews::drawListView(
            "##AddComponentList",
            componentItems,
            selectedAddComponentIndex,
            options
        );

        ImGui::EndCombo();
    }

    const ComponentAddEntry& selectedEntry = registry[static_cast<std::size_t>(selectedAddComponentIndex)];

    if (selectedEntry.needsScriptPath) {
        luaScripts = findLuaScripts();

        if (selectedScriptIndex < 0 ||
            selectedScriptIndex >= static_cast<int>(luaScripts.size())) {
            selectedScriptIndex = 0;
        }

        const char* scriptPreview = luaScripts.empty()
            ? "No Lua scripts found"
            : luaScripts[static_cast<std::size_t>(selectedScriptIndex)].c_str();

        ImGui::SetNextItemWidth(320.0f);
        if (ImGui::BeginCombo("Lua Script", scriptPreview)) {
            std::vector<ListViewItem> scriptItems;
            scriptItems.reserve(luaScripts.size());
            for (std::size_t index = 0; index < luaScripts.size(); ++index) {
                const std::string& scriptPath = luaScripts[index];
                scriptItems.push_back(ListViewItem{
                    scriptPath,
                    makeScriptListLabel(scriptPath),
                    scriptPath
                });
            }

            ListViewOptions options;
            options.height = 220.0f;
            options.border = false;
            options.emptyText = "No Lua scripts.";
            EditorCollectionViews::drawListView(
                "##LuaScriptList",
                scriptItems,
                selectedScriptIndex,
                options
            );

            ImGui::EndCombo();
        }

        if (luaScripts.empty()) {
            const std::filesystem::path scriptsPath =
                ProjectManager::getInstance().getAssetsPath() / "Scripts";
            ImGui::TextDisabled("Add .lua files under %s.", scriptsPath.string().c_str());
        } else {
            selectedScriptPath = luaScripts[static_cast<std::size_t>(selectedScriptIndex)];
        }
    }

    if (!selectedEntry.needsScriptPath) {
        ImGui::SameLine();
    }

    if (ImGui::Button("Add Component")) {
        const ComponentAddEntry& entry = registry[static_cast<std::size_t>(selectedAddComponentIndex)];
        if (entry.needsScriptPath && selectedScriptPath.empty()) {
            const std::filesystem::path scriptsPath =
                ProjectManager::getInstance().getAssetsPath() / "Scripts";
            statusMessage = "No Lua scripts found under " + scriptsPath.string() + ".";
        } else if (entry.add(entity, selectedScriptPath)) {
            entity.addInputListeners(InputSystem::getInstance());
            syncEditStateFromEntity(entity, true);
            statusMessage = "Added " + std::string(entry.name) + ".";
        } else {
            statusMessage = "Entity already has " + std::string(entry.name) + ".";
        }
    }
}

bool EntityInspectorPanel::drawRemoveComponentButton(
    Entity& entity,
    const char* componentName,
    std::string& statusMessage
) {
    if (selectedEntityId != entity.getId()) {
        return false;
    }

    const std::string buttonLabel = "Remove Component##" + std::string(componentName);
    if (ImGui::Button(buttonLabel.c_str())) {
        if (removeComponentByName(entity, componentName)) {
            syncEditStateFromEntity(entity, true);
            statusMessage = "Removed " + std::string(componentName) + ".";
            return true;
        }

        statusMessage = "Failed to remove " + std::string(componentName) + ".";
    }

    ImGui::Separator();
    return false;
}
