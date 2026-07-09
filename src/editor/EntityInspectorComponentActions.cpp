#include "editor/EntityInspectorPanel.h"

#include "Entity.h"
#include "components/CollisionComponent.h"
#include "components/AnimationComponent.h"
#include "components/CanvasComponent.h"
#include "components/PlayerCameraComponent.h"
#include "components/PlayerController.h"
#include "components/RectTransformComponent.h"
#include "components/ScriptComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "components/UILabelComponent.h"
#include "components/UIPanelComponent.h"
#include "editor/EditorCollectionViews.h"
#include "editor/ScriptPropertyParser.h"
#include "math/Vector2F.h"
#include "misc/Level.h"
#include "system/InputSystem.h"
#include "system/ProjectManager.h"
#include "system/Renderer.h"

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

Vector2F getViewportCenter() {
    const RenderRect viewport = Renderer::getInstance().getViewport();
    if (viewport.width <= 0.0f || viewport.height <= 0.0f) {
        return Vector2F(640.0f, 360.0f);
    }

    return Vector2F(
        viewport.x + (viewport.width * 0.5f),
        viewport.y + (viewport.height * 0.5f)
    );
}

Entity& findOrCreateCanvasEntity() {
    Level& level = Level::getCurrentLevel();
    for (Entity& entity : level.getEntities()) {
        if (!entity.isDestroyed() && entity.getComponent<CanvasComponent>() != nullptr) {
            return entity;
        }
    }

    Entity& canvas = level.createEntity();
    canvas.setName("Canvas");
    canvas.setTag("UI");
    canvas.addComponent<RectTransformComponent>(
        getViewportCenter(),
        Vector2F(640.0f, 320.0f),
        Vector2F(0.5f, 0.5f),
        0.0f
    );
    canvas.addComponent<CanvasComponent>();
    return canvas;
}

void ensureUiParent(Entity& entity) {
    if (entity.getComponent<CanvasComponent>() != nullptr) {
        return;
    }

    Entity& canvas = findOrCreateCanvasEntity();
    Entity* parent = entity.getParent();
    if (&entity != &canvas &&
        (parent == nullptr || parent->getComponent<CanvasComponent>() == nullptr)) {
        entity.setParent(&canvas, true);
    }
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
                std::error_code errorCode;
                const std::filesystem::path relativePath =
                    std::filesystem::relative(entry.path(), scriptsPath, errorCode);
                if (errorCode || relativePath.empty()) {
                    scripts.push_back(entry.path().generic_string());
                } else {
                    scripts.push_back(
                        (std::filesystem::path("Assets") / "Scripts" / relativePath).generic_string()
                    );
                }
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
    const std::string genericPath = path.generic_string();
    const std::string assetScriptsPrefix = "Assets/Scripts/";
    if (genericPath.rfind(assetScriptsPrefix, 0) == 0) {
        return genericPath.substr(assetScriptsPrefix.size());
    }

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
            "SpriteComponent",
            false,
            [](Entity& entity, const std::string& scriptPath) {
                (void)scriptPath;
                ensureComponent<TransformComponent>(entity, Vector2F(0.0f, 0.0f), 0.0f);

                const float renderScale = Renderer::getInstance().getRenderScale();
                Sprite sprite(nullptr, RenderRect{0.0f, 0.0f, 16.0f, 16.0f});
                sprite.setSize(Vector2F(16.0f * renderScale, 16.0f * renderScale));

                return addComponentIfMissing<SpriteComponent>(entity, sprite);
            }
        },
        {
            "AnimationComponent",
            false,
            [](Entity& entity, const std::string& scriptPath) {
                (void)scriptPath;
                ensureComponent<TransformComponent>(entity, Vector2F(0.0f, 0.0f), 0.0f);
                Animation animation(0.1f);
                return addComponentIfMissing<AnimationComponent>(entity, animation);
            }
        },
        {
            "PlayerCameraComponent",
            false,
            [](Entity& entity, const std::string& scriptPath) {
                (void)scriptPath;
                ensureComponent<TransformComponent>(entity, Vector2F(0.0f, 0.0f), 0.0f);
                entity.setTag("MainCamera");
                return addComponentIfMissing<PlayerCameraComponent>(entity);
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
            "UILabelComponent",
            false,
            [](Entity& entity, const std::string& scriptPath) {
                (void)scriptPath;
                ensureComponent<RectTransformComponent>(
                    entity,
                    getViewportCenter(),
                    Vector2F(240.0f, 48.0f),
                    Vector2F(0.5f, 0.5f),
                    0.0f
                );
                ensureUiParent(entity);

                if (entity.getComponent<UILabelComponent>() != nullptr) {
                    return false;
                }

                UILabelComponent& label = entity.addComponent<UILabelComponent>();
                label.setText("New Label");
                return true;
            }
        },
        {
            "UIPanelComponent",
            false,
            [](Entity& entity, const std::string& scriptPath) {
                (void)scriptPath;
                ensureComponent<RectTransformComponent>(
                    entity,
                    getViewportCenter(),
                    Vector2F(240.0f, 120.0f),
                    Vector2F(0.5f, 0.5f),
                    0.0f
                );
                ensureUiParent(entity);
                return addComponentIfMissing<UIPanelComponent>(entity);
            }
        },
        {
            "CanvasComponent",
            false,
            [](Entity& entity, const std::string& scriptPath) {
                (void)scriptPath;
                ensureComponent<RectTransformComponent>(entity);
                return addComponentIfMissing<CanvasComponent>(entity);
            }
        },
        {
            "RectTransformComponent",
            false,
            [](Entity& entity, const std::string& scriptPath) {
                (void)scriptPath;
                return addComponentIfMissing<RectTransformComponent>(entity);
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
                return target.removeComponent<CollisionComponent>();
            }
        });
    }

    if (entity.getComponent<PlayerCameraComponent>() != nullptr) {
        entries.push_back({
            "PlayerCameraComponent",
            [](Entity& target) {
                return target.removeComponent<PlayerCameraComponent>();
            }
        });
    }

    if (entity.getComponent<CanvasComponent>() != nullptr) {
        entries.push_back({
            "CanvasComponent",
            [](Entity& target) {
                return target.removeComponent<CanvasComponent>();
            }
        });
    }

    if (entity.getComponent<UILabelComponent>() != nullptr) {
        entries.push_back({
            "UILabelComponent",
            [](Entity& target) {
                return target.removeComponent<UILabelComponent>();
            }
        });
    }

    if (entity.getComponent<UIPanelComponent>() != nullptr) {
        entries.push_back({
            "UIPanelComponent",
            [](Entity& target) {
                return target.removeComponent<UIPanelComponent>();
            }
        });
    }

    if (entity.getComponent<RectTransformComponent>() != nullptr) {
        entries.push_back({
            "RectTransformComponent",
            [](Entity& target) {
                removeDependencyIfPresent<CanvasComponent>(target);
                removeDependencyIfPresent<UILabelComponent>(target);
                removeDependencyIfPresent<UIPanelComponent>(target);
                return target.removeComponent<RectTransformComponent>();
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
        options.closePopupOnSelection = true;
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
            options.closePopupOnSelection = true;
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
