#include "editor/EntityInspectorPanel.h"

#include "Entity.h"
#include "components/AnimationComponent.h"
#include "components/CollisionComponent.h"
#include "components/PlayerController.h"
#include "components/ScriptComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "game/Brick.h"
#include "game/Bullet.h"
#include "math/Vector2F.h"
#include "misc/Animation.h"
#include "misc/Level.h"
#include "misc/Sprite.h"
#include "serialization/PrefabSerializer.h"
#include "system/AssetManager.h"
#include "system/IRenderBackend.h"
#include "system/InputSystem.h"
#include "system/ProjectManager.h"

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
constexpr const char* EntityParentDragPayloadType = "IENGINE_ENTITY_PARENT";
constexpr const char* EntityReorderDragPayloadType = "IENGINE_ENTITY_REORDER";

Entity* findEntityById(Level& level, int entityId) {
    for (Entity& entity : level.getEntities()) {
        if (entity.getId() == entityId && !entity.isDestroyed()) {
            return &entity;
        }
    }

    return nullptr;
}

bool compareEntityDisplayOrder(const Entity* left, const Entity* right) {
    if (left == nullptr || right == nullptr) {
        return left != nullptr;
    }

    if (left->getDisplayOrder() != right->getDisplayOrder()) {
        return left->getDisplayOrder() < right->getDisplayOrder();
    }

    return left->getId() < right->getId();
}

std::vector<Entity*> getSortedRootEntities(Level& level) {
    std::vector<Entity*> result;

    for (Entity& entity : level.getEntities()) {
        if (!entity.isDestroyed() && entity.getParent() == nullptr) {
            result.push_back(&entity);
        }
    }

    std::sort(result.begin(), result.end(), compareEntityDisplayOrder);
    return result;
}

std::vector<Entity*> getSortedChildren(Entity& entity) {
    std::vector<Entity*> result;

    for (Entity* child : entity.getChildren()) {
        if (child != nullptr && !child->isDestroyed()) {
            result.push_back(child);
        }
    }

    std::sort(result.begin(), result.end(), compareEntityDisplayOrder);
    return result;
}

std::vector<Entity*> getSortedSiblings(Level& level, Entity* parent) {
    if (parent == nullptr) {
        return getSortedRootEntities(level);
    }

    return getSortedChildren(*parent);
}

void normalizeDisplayOrder(std::vector<Entity*>& entities) {
    for (std::size_t index = 0; index < entities.size(); ++index) {
        if (entities[index] != nullptr) {
            entities[index]->setDisplayOrder(static_cast<int>(index));
        }
    }
}

bool moveEntityDisplayOrder(
    Level& level,
    Entity& draggedEntity,
    Entity& targetEntity,
    bool beforeTarget
) {
    if (&draggedEntity == &targetEntity) {
        return false;
    }

    if (draggedEntity.getParent() != targetEntity.getParent()) {
        return false;
    }

    std::vector<Entity*> siblings = getSortedSiblings(level, targetEntity.getParent());
    auto draggedIterator = std::find(siblings.begin(), siblings.end(), &draggedEntity);
    auto targetIterator = std::find(siblings.begin(), siblings.end(), &targetEntity);
    if (draggedIterator == siblings.end() || targetIterator == siblings.end()) {
        return false;
    }

    Entity* dragged = *draggedIterator;
    siblings.erase(draggedIterator);

    targetIterator = std::find(siblings.begin(), siblings.end(), &targetEntity);
    if (targetIterator == siblings.end()) {
        siblings.push_back(dragged);
    } else {
        if (!beforeTarget) {
            ++targetIterator;
        }

        siblings.insert(targetIterator, dragged);
    }

    normalizeDisplayOrder(siblings);
    return true;
}

void moveEntityToSiblingEnd(Level& level, Entity& entity) {
    std::vector<Entity*> siblings = getSortedSiblings(level, entity.getParent());
    siblings.erase(
        std::remove(siblings.begin(), siblings.end(), &entity),
        siblings.end()
    );
    siblings.push_back(&entity);
    normalizeDisplayOrder(siblings);
}

void moveEntityToRootEnd(Level& level, Entity& entity) {
    entity.clearParent();
    moveEntityToSiblingEnd(level, entity);
}

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
    std::filesystem::path path =
        ProjectManager::getInstance().getAssetsPath() / "Prefabs" / sanitizePrefabName(prefabName);
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
    bool needsScriptPath = false;
    std::function<bool(Entity&, const std::string&)> add;
};

struct ComponentRemoveEntry {
    const char* name;
    std::function<bool(Entity&)> remove;
};

template <typename TComponent>
void removeDependencyIfPresent(Entity& entity) {
    if (entity.getComponent<TComponent>() != nullptr) {
        entity.removeComponent<TComponent>();
    }
}

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
                if (scriptPath.empty()) {
                    return false;
                }

                return addComponentIfMissing<ScriptComponent>(entity, scriptPath);
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

        if (ImGui::BeginDragDropTarget()) {
            const ImGuiPayload* reorderPayload = ImGui::AcceptDragDropPayload(EntityReorderDragPayloadType);
            if (reorderPayload != nullptr && reorderPayload->IsDelivery() && reorderPayload->DataSize == sizeof(int)) {
                const int draggedEntityId = *static_cast<const int*>(reorderPayload->Data);
                Entity* draggedEntity = findEntityById(level, draggedEntityId);
                if (draggedEntity != nullptr) {
                    moveEntityToRootEnd(level, *draggedEntity);
                    syncEditStateFromEntity(*draggedEntity, true);
                    statusMessage = "Moved entity " + std::to_string(draggedEntity->getId()) + " to root order.";
                }
            } else {
                const ImGuiPayload* parentPayload = ImGui::AcceptDragDropPayload(EntityParentDragPayloadType);
                if (parentPayload != nullptr && parentPayload->IsDelivery() && parentPayload->DataSize == sizeof(int)) {
                    const int draggedEntityId = *static_cast<const int*>(parentPayload->Data);
                Entity* draggedEntity = findEntityById(level, draggedEntityId);
                if (draggedEntity != nullptr) {
                    draggedEntity->clearParent();
                    moveEntityToSiblingEnd(level, *draggedEntity);
                    syncEditStateFromEntity(*draggedEntity, true);
                    statusMessage = "Cleared parent for entity " + std::to_string(draggedEntity->getId()) + ".";
                }
                }
            }

            ImGui::EndDragDropTarget();
        }

        if (levelOpen) {
            std::vector<Entity*> rootEntities = getSortedRootEntities(level);
            for (Entity* entity : rootEntities) {
                if (entity != nullptr) {
                    drawEntityTreeNode(*entity, statusMessage);
                }
            }

            ImGui::TreePop();
        }
    }

    ImGui::EndChild();
    processPendingDelete(level, statusMessage);
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

void EntityInspectorPanel::requestDeleteSelected(std::string& statusMessage) {
    if (selectedEntityId < 0) {
        statusMessage = "No entity selected.";
        return;
    }

    pendingDeleteEntityId = selectedEntityId;
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

    if (ImGui::SmallButton("::")) {
        selectEntity(entity);
    }

    if (ImGui::BeginDragDropSource()) {
        const int entityId = entity.getId();
        ImGui::SetDragDropPayload(EntityReorderDragPayloadType, &entityId, sizeof(entityId));
        ImGui::Text("Move: %s", entity.getName().c_str());
        ImGui::EndDragDropSource();
    }

    ImGui::SameLine();
    const bool open = ImGui::TreeNodeEx(label.c_str(), flags);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        selectEntity(entity);
    }

    if (ImGui::BeginDragDropSource()) {
        const int entityId = entity.getId();
        ImGui::SetDragDropPayload(EntityParentDragPayloadType, &entityId, sizeof(entityId));
        ImGui::Text("Entity: %s", entity.getName().c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
        Level& level = Level::getCurrentLevel();
        const ImGuiPayload* reorderPayload = ImGui::AcceptDragDropPayload(EntityReorderDragPayloadType);
        if (reorderPayload != nullptr && reorderPayload->IsDelivery() && reorderPayload->DataSize == sizeof(int)) {
            const int draggedEntityId = *static_cast<const int*>(reorderPayload->Data);
            Entity* draggedEntity = findEntityById(Level::getCurrentLevel(), draggedEntityId);
            if (draggedEntity != nullptr && draggedEntity != &entity) {
                const ImVec2 itemMin = ImGui::GetItemRectMin();
                const ImVec2 itemMax = ImGui::GetItemRectMax();
                const bool beforeTarget = ImGui::GetMousePos().y < ((itemMin.y + itemMax.y) * 0.5f);
                if (moveEntityDisplayOrder(level, *draggedEntity, entity, beforeTarget)) {
                    syncEditStateFromEntity(*draggedEntity, true);
                    statusMessage =
                        "Reordered entity " + std::to_string(draggedEntity->getId())
                        + (beforeTarget ? " before " : " after ")
                        + std::to_string(entity.getId()) + ".";
                } else {
                    statusMessage = "Can only reorder entities with the same parent.";
                }
            }
        } else {
            const ImGuiPayload* parentPayload = ImGui::AcceptDragDropPayload(EntityParentDragPayloadType);
            if (parentPayload != nullptr && parentPayload->IsDelivery() && parentPayload->DataSize == sizeof(int)) {
                const int draggedEntityId = *static_cast<const int*>(parentPayload->Data);
                Entity* draggedEntity = findEntityById(Level::getCurrentLevel(), draggedEntityId);
                if (draggedEntity != nullptr && draggedEntity != &entity) {
                    if (draggedEntity->setParent(&entity)) {
                        moveEntityToSiblingEnd(level, *draggedEntity);
                        syncEditStateFromEntity(*draggedEntity, true);
                        statusMessage =
                            "Parented entity " + std::to_string(draggedEntity->getId())
                            + " to entity " + std::to_string(entity.getId()) + ".";
                    } else {
                        statusMessage = "Cannot parent entity there.";
                    }
                }
            }
        }

        ImGui::EndDragDropTarget();
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
        ImGui::Text("Enabled: %s", entity.isEnabled() ? "Yes" : "No");

        if (selectedEntityId == entity.getId()) {
            syncEditStateFromEntity(entity);
            drawEntityIdentityFields(entity, statusMessage);
        } else {
            ImGui::Text("Name: %s", entity.getName().c_str());
            ImGui::Text("Tag: %s", entity.getTag().c_str());
        }

        drawEntityComponents(entity, statusMessage);

        std::vector<Entity*> children = getSortedChildren(entity);
        if (!children.empty()
            && ImGui::TreeNodeEx(
                "Children",
                ImGuiTreeNodeFlags_DefaultOpen |
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_SpanAvailWidth)) {
            for (Entity* child : children) {
                if (child != nullptr) {
                    drawEntityTreeNode(*child, statusMessage);
                }
            }

            ImGui::TreePop();
        }

        if (ImGui::SmallButton("Delete Entity")) {
            pendingDeleteEntityId = entity.getId();
        }

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
            if (drawRemoveComponentButton(entity, "TransformComponent", statusMessage)) {
                ImGui::TreePop();
                ImGui::TreePop();
                return;
            }

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
            if (drawRemoveComponentButton(entity, "SpriteComponent", statusMessage)) {
                ImGui::TreePop();
                ImGui::TreePop();
                return;
            }

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
            if (drawRemoveComponentButton(entity, "AnimationComponent", statusMessage)) {
                ImGui::TreePop();
                ImGui::TreePop();
                return;
            }

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
            if (drawRemoveComponentButton(entity, "CollisionComponent", statusMessage)) {
                ImGui::TreePop();
                ImGui::TreePop();
                return;
            }

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
            if (drawRemoveComponentButton(entity, "ScriptComponent", statusMessage)) {
                ImGui::TreePop();
                ImGui::TreePop();
                return;
            }

            drawScriptComponentFields(*scriptComponent);
            ImGui::TreePop();
        }
    }

    if (entity.getComponent<PlayerController>() != nullptr) {
        hasComponents = true;
        if (ImGui::TreeNodeEx("PlayerController", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth)) {
            if (drawRemoveComponentButton(entity, "PlayerController", statusMessage)) {
                ImGui::TreePop();
                ImGui::TreePop();
                return;
            }

            ImGui::TextUnformatted("No editable fields.");
            ImGui::TreePop();
        }
    }

    if (entity.getComponent<Brick>() != nullptr) {
        hasComponents = true;
        if (ImGui::TreeNodeEx("Brick", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth)) {
            if (drawRemoveComponentButton(entity, "Brick", statusMessage)) {
                ImGui::TreePop();
                ImGui::TreePop();
                return;
            }

            ImGui::TextUnformatted("No editable fields.");
            ImGui::TreePop();
        }
    }

    if (entity.getComponent<Bullet>() != nullptr) {
        hasComponents = true;
        if (ImGui::TreeNodeEx("Bullet", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth)) {
            if (drawRemoveComponentButton(entity, "Bullet", statusMessage)) {
                ImGui::TreePop();
                ImGui::TreePop();
                return;
            }

            ImGui::Text("Speed: %.2f", entity.getComponent<Bullet>()->speed);
            ImGui::TreePop();
        }
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
    std::vector<std::string> luaScripts;
    std::string selectedScriptPath;

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
            for (std::size_t index = 0; index < luaScripts.size(); ++index) {
                const bool selected = selectedScriptIndex == static_cast<int>(index);
                if (ImGui::Selectable(luaScripts[index].c_str(), selected)) {
                    selectedScriptIndex = static_cast<int>(index);
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

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

    ImGui::Checkbox("Enabled", &entityEditState.enabled);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        entity.setEnabled(entityEditState.enabled);
        statusMessage = "Updated entity enabled state.";
    }

    if (Entity* parent = entity.getParent()) {
        ImGui::Text("Parent: %s (%d)", parent->getName().c_str(), parent->getId());
        if (ImGui::Button("Clear Parent")) {
            entity.clearParent();
            syncEditStateFromEntity(entity, true);
            statusMessage = "Cleared entity parent.";
        }
    } else {
        ImGui::TextUnformatted("Parent: none");
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

void EntityInspectorPanel::processPendingDelete(Level& level, std::string& statusMessage) {
    if (pendingDeleteEntityId < 0) {
        return;
    }

    const int deletedId = pendingDeleteEntityId;
    pendingDeleteEntityId = -1;

    if (!level.destroyEntityByIdHierarchy(deletedId)) {
        statusMessage = "Failed to delete entity " + std::to_string(deletedId) + ".";
        return;
    }

    level.cleanupDestroyedEntities();

    if (selectedEntityId >= 0 && findEntityById(level, selectedEntityId) == nullptr) {
        clearSelection();
    }

    statusMessage = "Deleted entity " + std::to_string(deletedId) + ".";
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

    ImGui::InputFloat2("Offset", entityEditState.collisionOffset, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        collisionComponent.setOffset(Vector2F(
            entityEditState.collisionOffset[0],
            entityEditState.collisionOffset[1]
        ));
        statusMessage = "Updated CollisionComponent offset.";
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
    entityEditState.enabled = entity.isEnabled();

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
        const Vector2F& offset = collisionComponent->getOffset();
        entityEditState.collisionOffset[0] = offset.x;
        entityEditState.collisionOffset[1] = offset.y;
        entityEditState.collisionSize[0] = collisionComponent->getWidth();
        entityEditState.collisionSize[1] = collisionComponent->getHeight();
        entityEditState.collisionBodyType = bodyTypeToIndex(collisionComponent->getBodyType());
        entityEditState.collisionSensor = collisionComponent->isSensor();
        entityEditState.collisionName = collisionComponent->getName();
    }
    entityEditState.enabled = entity.isEnabled();
}
