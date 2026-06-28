#include "editor/EntityInspectorPanel.h"

#include "Entity.h"
#include "components/AnimationComponent.h"
#include "components/Component.h"
#include "components/CollisionComponent.h"
#include "components/PlayerController.h"
#include "components/ScriptComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "editor/EditorCollectionViews.h"
#include "editor/ScriptPropertyParser.h"
#include "game/Brick.h"
#include "math/Vector2F.h"
#include "misc/Animation.h"
#include "misc/Level.h"
#include "misc/Sprite.h"
#include "serialization/PrefabSerializer.h"
#include "system/AssetManager.h"
#include "system/IRenderBackend.h"
#include "system/InputSystem.h"
#include "system/ProjectManager.h"
#include "system/Renderer.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <functional>
#include <optional>
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

void drawComponentEnabledCheckbox(
    Component& component,
    const char* componentName,
    std::string& statusMessage
) {
    bool enabled = component.isEnabled();
    if (ImGui::Checkbox("Enabled", &enabled)) {
        component.setEnabled(enabled);
        statusMessage = std::string(componentName)
            + (enabled ? " enabled." : " disabled.");
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

}

bool EntityInspectorPanel::draw(std::string& statusMessage) {
    Level& level = Level::getCurrentLevel();
    auto& entities = level.getEntities();
    bool prefabSaved = false;

    const std::string levelFileName = level.getSourceFileName();
    const std::string levelLabel = levelFileName.empty()
        ? "Current Level"
        : "Current Level (" + levelFileName + ")";

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
            levelLabel.c_str(),
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

    if (entity.isPersistent()) {
        label += " (Persistent)";
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

            drawComponentEnabledCheckbox(*transform, "TransformComponent", statusMessage);

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

            drawComponentEnabledCheckbox(*spriteComponent, "SpriteComponent", statusMessage);

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

            drawComponentEnabledCheckbox(*animationComponent, "AnimationComponent", statusMessage);

            if (editable) {
                drawAnimationComponentFields(*animationComponent, statusMessage);
            } else {
                const Animation& animation = animationComponent->getAnimation();

                ImGui::Text("Frames: %zu", animation.getFrameCount());
                ImGui::Text("Current Frame: %zu", animationComponent->getCurrentFrameIndex());
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

            drawComponentEnabledCheckbox(*collisionComponent, "CollisionComponent", statusMessage);

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

            drawComponentEnabledCheckbox(*scriptComponent, "ScriptComponent", statusMessage);

            drawScriptComponentFields(*scriptComponent, statusMessage);
            ImGui::TreePop();
        }
    }

    if (auto* playerController = entity.getComponent<PlayerController>()) {
        hasComponents = true;
        if (ImGui::TreeNodeEx("PlayerController", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth)) {
            if (drawRemoveComponentButton(entity, "PlayerController", statusMessage)) {
                ImGui::TreePop();
                ImGui::TreePop();
                return;
            }

            drawComponentEnabledCheckbox(*playerController, "PlayerController", statusMessage);
            ImGui::TextUnformatted("No editable fields.");
            ImGui::TreePop();
        }
    }

    if (auto* brick = entity.getComponent<Brick>()) {
        hasComponents = true;
        if (ImGui::TreeNodeEx("Brick", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth)) {
            if (drawRemoveComponentButton(entity, "Brick", statusMessage)) {
                ImGui::TreePop();
                ImGui::TreePop();
                return;
            }

            drawComponentEnabledCheckbox(*brick, "Brick", statusMessage);
            ImGui::TextUnformatted("No editable fields.");
            ImGui::TreePop();
        }
    }

    if (!hasComponents) {
        ImGui::TextUnformatted("No known components.");
    }

    ImGui::TreePop();
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

void EntityInspectorPanel::syncEditStateFromEntity(Entity& entity, bool force) {
    if (!force && entityEditState.entityId == entity.getId()) {
        return;
    }

    entityEditState = EntityEditState{};
    entityEditState.entityId = entity.getId();
    entityEditState.name = entity.getName();
    entityEditState.tag = entity.getTag();
    entityEditState.enabled = entity.isEnabled();
    selectedAnimationFileIndex = -1;

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
        entityEditState.animationLooping = animationComponent->isLooping();
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
