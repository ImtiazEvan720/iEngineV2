#include "editor/EntityInspectorPanel.h"

#include "Entity.h"
#include "components/AnimationComponent.h"
#include "components/CollisionComponent.h"
#include "components/PlayerController.h"
#include "components/ScriptComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "editor/PrefabSerializer.h"
#include "game/Brick.h"
#include "game/Bullet.h"
#include "math/Vector2F.h"
#include "misc/Animation.h"
#include "misc/Level.h"
#include "misc/Sprite.h"
#include "system/AssetManager.h"
#include "system/IRenderBackend.h"
#include "system/InputSystem.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {
int bodyTypeToIndex(CollisionComponent::BodyType bodyType) {
    switch (bodyType) {
        case CollisionComponent::BodyType::Kinematic:
            return 1;
        case CollisionComponent::BodyType::Dynamic:
            return 2;
        case CollisionComponent::BodyType::Static:
        default:
            return 0;
    }
}

CollisionComponent::BodyType bodyTypeFromIndex(int index) {
    switch (index) {
        case 1:
            return CollisionComponent::BodyType::Kinematic;
        case 2:
            return CollisionComponent::BodyType::Dynamic;
        case 0:
        default:
            return CollisionComponent::BodyType::Static;
    }
}

std::string sanitizePrefabName(const std::string& value) {
    std::string sanitized;
    sanitized.reserve(value.size());

    for (char character : value) {
        const unsigned char unsignedCharacter = static_cast<unsigned char>(character);
        if (std::isalnum(unsignedCharacter) || character == '-' || character == '_') {
            sanitized.push_back(character);
        } else if (std::isspace(unsignedCharacter)) {
            sanitized.push_back('_');
        }
    }

    if (sanitized.empty()) {
        sanitized = "Prefab";
    }

    return sanitized;
}

std::filesystem::path getPrefabPath(const std::string& prefabName) {
    std::filesystem::path path = std::filesystem::path("Assets") / "Prefabs" / sanitizePrefabName(prefabName);
    path.replace_extension(".iprefab");
    return path;
}

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

struct ComponentAddEntry {
    const char* name;
    std::function<bool(Entity&)> add;
};

const std::vector<ComponentAddEntry>& getComponentAddRegistry() {
    static const std::vector<ComponentAddEntry> registry = {
        {
            "TransformComponent",
            [](Entity& entity) {
                return addComponentIfMissing<TransformComponent>(
                    entity,
                    Vector2F(0.0f, 0.0f),
                    0.0f
                );
            }
        },
        {
            "CollisionComponent",
            [](Entity& entity) {
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
            "PlayerController",
            [](Entity& entity) {
                ensureComponent<TransformComponent>(entity, Vector2F(0.0f, 0.0f), 0.0f);
                return addComponentIfMissing<PlayerController>(entity);
            }
        },
        {
            "Brick",
            [](Entity& entity) {
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
            [](Entity& entity) {
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
}

bool EntityInspectorPanel::draw(std::string& statusMessage) {
    Level& level = Level::getCurrentLevel();
    auto& entities = level.getEntities();
    bool prefabSaved = false;

    ImGui::Text("Entities: %zu", entities.size());

    if (selectedEntityId >= 0) {
        ImGui::SameLine();
        ImGui::Text("Selected: %d", selectedEntityId);
    }

    if (!statusMessage.empty()) {
        ImGui::TextWrapped("%s", statusMessage.c_str());
    }

    ImGui::Separator();
    prefabSaved = drawPrefabActions(level, statusMessage);
    ImGui::Separator();

    if (entities.empty()) {
        ImGui::TextUnformatted("Current level has no entities.");
        return prefabSaved;
    }

    if (ImGui::BeginChild(
            "##EntityTree",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_Borders,
            ImGuiWindowFlags_HorizontalScrollbar)) {
        const bool levelOpen = ImGui::TreeNodeEx(
            "Current Level",
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth
        );

        if (levelOpen) {
            for (Entity& entity : entities) {
                drawEntityTreeNode(entity, statusMessage);
            }

            ImGui::TreePop();
        }
    }

    ImGui::EndChild();
    return prefabSaved;
}

int EntityInspectorPanel::getSelectedEntityId() const {
    return selectedEntityId;
}

void EntityInspectorPanel::selectEntity(Entity& entity, bool forceSync) {
    const bool changedSelection = selectedEntityId != entity.getId();
    selectedEntityId = entity.getId();
    if (changedSelection || prefabName.empty()) {
        prefabName = sanitizePrefabName(entity.getName());
    }

    syncEditStateFromEntity(entity, forceSync);
}

void EntityInspectorPanel::clearSelection() {
    selectedEntityId = -1;
    prefabName.clear();
    entityEditState = EntityEditState{};
}

void EntityInspectorPanel::drawEntityTreeNode(Entity& entity, std::string& statusMessage) {
    ImGui::PushID(entity.getId());

    std::string label = entity.getName();
    if (label.empty()) {
        label = "Entity";
    }

    label += "##" + std::to_string(entity.getId());

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_OpenOnDoubleClick |
        ImGuiTreeNodeFlags_SpanAvailWidth;

    if (selectedEntityId == entity.getId()) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    const bool open = ImGui::TreeNodeEx(label.c_str(), flags);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        selectEntity(entity);
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "id: %d\nname: %s\ntag: %s",
            entity.getId(),
            entity.getName().c_str(),
            entity.getTag().c_str()
        );
    }

    if (open) {
        ImGui::Text("Id: %d", entity.getId());
        ImGui::Text("State: %s", entity.isDestroyed() ? "Destroyed" : "Active");

        if (selectedEntityId == entity.getId()) {
            syncEditStateFromEntity(entity);
            drawEntityIdentityFields(entity, statusMessage);
        } else {
            ImGui::Text("Name: %s", entity.getName().c_str());
            ImGui::Text("Tag: %s", entity.getTag().c_str());
        }

        drawEntityComponents(entity, statusMessage);
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void EntityInspectorPanel::drawEntityComponents(Entity& entity, std::string& statusMessage) {
    if (!ImGui::TreeNodeEx(
            "Components",
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth)) {
        return;
    }

    bool hasComponents = false;
    const bool editable = selectedEntityId == entity.getId();

    if (editable) {
        drawAddComponentCombo(entity, statusMessage);
        ImGui::Separator();
    }

    if (auto* transform = entity.getComponent<TransformComponent>()) {
        hasComponents = true;
        if (ImGui::TreeNodeEx("TransformComponent", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth)) {
            if (editable) {
                drawTransformComponentFields(*transform, statusMessage);
            } else {
                const Vector2F& position = transform->getPosition();
                const Vector2F worldPosition = transform->getWorldPosition();

                ImGui::Text("Position: %.2f, %.2f", position.x, position.y);
                ImGui::Text("World Position: %.2f, %.2f", worldPosition.x, worldPosition.y);
                ImGui::Text("Rotation: %.2f", transform->getRotation());
                ImGui::Text("World Rotation: %.2f", transform->getWorldRotation());
            }

            ImGui::TreePop();
        }
    }

    if (auto* spriteComponent = entity.getComponent<SpriteComponent>()) {
        hasComponents = true;
        if (ImGui::TreeNodeEx("SpriteComponent", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth)) {
            if (editable) {
                drawSpriteComponentFields(*spriteComponent, statusMessage);
            } else {
                const Sprite& sprite = spriteComponent->getSprite();
                const RenderRect& source = sprite.getSourceRect();
                const Vector2F& size = sprite.getSize();
                const Vector2F& origin = sprite.getOrigin();

                ImGui::Text("Source: %.2f, %.2f, %.2f, %.2f", source.x, source.y, source.width, source.height);
                ImGui::Text("Size: %.2f, %.2f", size.x, size.y);
                ImGui::Text("Origin: %.2f, %.2f", origin.x, origin.y);
            }

            ImGui::TreePop();
        }
    }

    if (auto* animationComponent = entity.getComponent<AnimationComponent>()) {
        hasComponents = true;
        if (ImGui::TreeNodeEx("AnimationComponent", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth)) {
            if (editable) {
                drawAnimationComponentFields(*animationComponent, statusMessage);
            } else {
                const Animation& animation = animationComponent->getAnimation();

                ImGui::Text("Frames: %zu", animation.getFrameCount());
                ImGui::Text("Frame Duration: %.3f", animation.getFrameDuration());
                ImGui::Text("Playing: %s", animationComponent->isPlaying() ? "true" : "false");
                ImGui::Text("Finished: %s", animationComponent->isFinished() ? "true" : "false");
            }

            ImGui::TreePop();
        }
    }

    if (auto* collisionComponent = entity.getComponent<CollisionComponent>()) {
        hasComponents = true;
        if (ImGui::TreeNodeEx("CollisionComponent", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth)) {
            if (editable) {
                drawCollisionComponentFields(*collisionComponent, statusMessage);
            } else {
                ImGui::Text("Name: %s", collisionComponent->getName().c_str());
                ImGui::Text("Size: %.2f, %.2f", collisionComponent->getWidth(), collisionComponent->getHeight());
                ImGui::Text("Sensor: %s", collisionComponent->isSensor() ? "true" : "false");
            }

            ImGui::TreePop();
        }
    }

    if (auto* scriptComponent = entity.getComponent<ScriptComponent>()) {
        hasComponents = true;
        if (ImGui::TreeNodeEx("ScriptComponent", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth)) {
            drawScriptComponentFields(*scriptComponent);
            ImGui::TreePop();
        }
    }

    if (entity.getComponent<PlayerController>() != nullptr) {
        hasComponents = true;
        ImGui::BulletText("PlayerController");
    }

    if (entity.getComponent<Brick>() != nullptr) {
        hasComponents = true;
        ImGui::BulletText("Brick");
    }

    if (entity.getComponent<Bullet>() != nullptr) {
        hasComponents = true;
        ImGui::BulletText("Bullet");
    }

    if (!hasComponents) {
        ImGui::TextUnformatted("No known components.");
    }

    ImGui::TreePop();
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

    const ComponentAddEntry& selectedEntry = registry[static_cast<std::size_t>(selectedAddComponentIndex)];

    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::BeginCombo("Component", selectedEntry.name)) {
        for (std::size_t index = 0; index < registry.size(); ++index) {
            const bool selected = selectedAddComponentIndex == static_cast<int>(index);
            if (ImGui::Selectable(registry[index].name, selected)) {
                selectedAddComponentIndex = static_cast<int>(index);
            }

            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if (ImGui::Button("Add Component")) {
        const ComponentAddEntry& entry = registry[static_cast<std::size_t>(selectedAddComponentIndex)];
        if (entry.add(entity)) {
            entity.addInputListeners(InputSystem::getInstance());
            syncEditStateFromEntity(entity, true);
            statusMessage = "Added " + std::string(entry.name) + ".";
        } else {
            statusMessage = "Entity already has " + std::string(entry.name) + ".";
        }
    }
}

void EntityInspectorPanel::drawEntityIdentityFields(Entity& entity, std::string& statusMessage) {
    ImGui::InputText("Name", &entityEditState.name);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        entity.setName(entityEditState.name);
        statusMessage = "Updated entity name.";
    }

    ImGui::InputText("Tag", &entityEditState.tag);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        entity.setTag(entityEditState.tag);
        statusMessage = "Updated entity tag.";
    }
}

bool EntityInspectorPanel::drawPrefabActions(Level& level, std::string& statusMessage) {
    Entity* selectedEntity = findSelectedEntity(level);
    if (selectedEntity == nullptr) {
        ImGui::BeginDisabled();
        ImGui::InputText("Prefab Name", &prefabName);
        ImGui::Button("Save As Prefab");
        ImGui::EndDisabled();
        ImGui::TextUnformatted("Select an entity to save it as a prefab.");
        return false;
    }

    if (prefabName.empty()) {
        prefabName = sanitizePrefabName(selectedEntity->getName());
    }

    ImGui::InputText("Prefab Name", &prefabName);
    ImGui::SameLine();

    if (ImGui::Button("Save As Prefab")) {
        const std::filesystem::path path = getPrefabPath(prefabName);
        std::string errorMessage;

        if (PrefabSerializer::saveEntity(*selectedEntity, path.string(), errorMessage)) {
            if (AssetManager::getInstance().loadAssetFile(path.string())) {
                statusMessage = "Saved prefab: " + path.string();
            } else {
                statusMessage = "Saved prefab, but failed to register it with AssetManager: " + path.string();
            }

            return true;
        }

        statusMessage = errorMessage.empty() ? "Failed to save prefab." : errorMessage;
    }

    ImGui::TextWrapped("Output: %s", getPrefabPath(prefabName).string().c_str());
    return false;
}

Entity* EntityInspectorPanel::findSelectedEntity(Level& level) const {
    if (selectedEntityId < 0) {
        return nullptr;
    }

    for (Entity& entity : level.getEntities()) {
        if (entity.getId() == selectedEntityId && !entity.isDestroyed()) {
            return &entity;
        }
    }

    return nullptr;
}

void EntityInspectorPanel::drawTransformComponentFields(
    TransformComponent& transform,
    std::string& statusMessage
) {
    ImGui::InputFloat2("Position", entityEditState.position, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        transform.setPosition(Vector2F(entityEditState.position[0], entityEditState.position[1]));
        statusMessage = "Updated TransformComponent position.";
    }

    ImGui::InputFloat("Rotation", &entityEditState.rotation, 0.0f, 0.0f, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        transform.setRotation(entityEditState.rotation);
        statusMessage = "Updated TransformComponent rotation.";
    }

    const Vector2F worldPosition = transform.getWorldPosition();
    ImGui::Text("World Position: %.2f, %.2f", worldPosition.x, worldPosition.y);
    ImGui::Text("World Rotation: %.2f", transform.getWorldRotation());
}

void EntityInspectorPanel::drawSpriteComponentFields(
    SpriteComponent& spriteComponent,
    std::string& statusMessage
) {
    Sprite& sprite = spriteComponent.getSprite();

    ImGui::InputFloat4("Source Rect", entityEditState.spriteSource, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        sprite.setSourceRect(RenderRect{
            entityEditState.spriteSource[0],
            entityEditState.spriteSource[1],
            entityEditState.spriteSource[2],
            entityEditState.spriteSource[3]
        });
        statusMessage = "Updated SpriteComponent source rect.";
    }

    ImGui::InputFloat2("Size", entityEditState.spriteSize, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        sprite.setSize(Vector2F(entityEditState.spriteSize[0], entityEditState.spriteSize[1]));
        statusMessage = "Updated SpriteComponent size.";
    }

    ImGui::InputFloat2("Origin", entityEditState.spriteOrigin, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        sprite.setOrigin(Vector2F(entityEditState.spriteOrigin[0], entityEditState.spriteOrigin[1]));
        statusMessage = "Updated SpriteComponent origin.";
    }
}

void EntityInspectorPanel::drawAnimationComponentFields(
    AnimationComponent& animationComponent,
    std::string& statusMessage
) {
    Animation& animation = animationComponent.getAnimation();

    ImGui::Text("Frames: %zu", animation.getFrameCount());
    ImGui::Text("Finished: %s", animationComponent.isFinished() ? "true" : "false");

    ImGui::InputFloat("Frame Duration", &entityEditState.animationFrameDuration, 0.0f, 0.0f, "%.3f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        entityEditState.animationFrameDuration = std::max(0.001f, entityEditState.animationFrameDuration);
        animation.setFrameDuration(entityEditState.animationFrameDuration);
        statusMessage = "Updated AnimationComponent frame duration.";
    }

    if (ImGui::Checkbox("Playing", &entityEditState.animationPlaying)) {
        if (entityEditState.animationPlaying) {
            animationComponent.play();
        } else {
            animationComponent.pause();
        }

        statusMessage = "Updated AnimationComponent playback.";
    }
}

void EntityInspectorPanel::drawCollisionComponentFields(
    CollisionComponent& collisionComponent,
    std::string& statusMessage
) {
    ImGui::InputText("Name", &entityEditState.collisionName);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        collisionComponent.setName(entityEditState.collisionName);
        statusMessage = "Updated CollisionComponent name.";
    }

    ImGui::InputFloat2("Size", entityEditState.collisionSize, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        entityEditState.collisionSize[0] = std::max(0.001f, entityEditState.collisionSize[0]);
        entityEditState.collisionSize[1] = std::max(0.001f, entityEditState.collisionSize[1]);
        collisionComponent.setSize(entityEditState.collisionSize[0], entityEditState.collisionSize[1]);
        statusMessage = "Updated CollisionComponent size.";
    }

    if (ImGui::Combo("Body Type", &entityEditState.collisionBodyType, "Static\0Kinematic\0Dynamic\0")) {
        collisionComponent.setBodyType(bodyTypeFromIndex(entityEditState.collisionBodyType));
        statusMessage = "Updated CollisionComponent body type.";
    }

    if (ImGui::Checkbox("Sensor", &entityEditState.collisionSensor)) {
        collisionComponent.setSensor(entityEditState.collisionSensor);
        statusMessage = "Updated CollisionComponent sensor.";
    }
}

void EntityInspectorPanel::drawScriptComponentFields(ScriptComponent& scriptComponent) {
    ImGui::TextWrapped("Path: %s", scriptComponent.getScriptPath().c_str());
    ImGui::Text("Loaded: %s", scriptComponent.isLoaded() ? "true" : "false");
}

void EntityInspectorPanel::syncEditStateFromEntity(Entity& entity, bool force) {
    if (!force && entityEditState.entityId == entity.getId()) {
        return;
    }

    entityEditState = EntityEditState{};
    entityEditState.entityId = entity.getId();
    entityEditState.name = entity.getName();
    entityEditState.tag = entity.getTag();

    if (TransformComponent* transform = entity.getComponent<TransformComponent>()) {
        const Vector2F& position = transform->getPosition();
        entityEditState.position[0] = position.x;
        entityEditState.position[1] = position.y;
        entityEditState.rotation = transform->getRotation();
    }

    if (SpriteComponent* spriteComponent = entity.getComponent<SpriteComponent>()) {
        const Sprite& sprite = spriteComponent->getSprite();
        const RenderRect& source = sprite.getSourceRect();
        const Vector2F& size = sprite.getSize();
        const Vector2F& origin = sprite.getOrigin();

        entityEditState.spriteSource[0] = source.x;
        entityEditState.spriteSource[1] = source.y;
        entityEditState.spriteSource[2] = source.width;
        entityEditState.spriteSource[3] = source.height;
        entityEditState.spriteSize[0] = size.x;
        entityEditState.spriteSize[1] = size.y;
        entityEditState.spriteOrigin[0] = origin.x;
        entityEditState.spriteOrigin[1] = origin.y;
    }

    if (AnimationComponent* animationComponent = entity.getComponent<AnimationComponent>()) {
        entityEditState.animationFrameDuration = animationComponent->getAnimation().getFrameDuration();
        entityEditState.animationPlaying = animationComponent->isPlaying();
    }

    if (CollisionComponent* collisionComponent = entity.getComponent<CollisionComponent>()) {
        entityEditState.collisionSize[0] = collisionComponent->getWidth();
        entityEditState.collisionSize[1] = collisionComponent->getHeight();
        entityEditState.collisionBodyType = bodyTypeToIndex(collisionComponent->getBodyType());
        entityEditState.collisionSensor = collisionComponent->isSensor();
        entityEditState.collisionName = collisionComponent->getName();
    }
}
