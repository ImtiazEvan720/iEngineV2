#include "editor/EntityTreePanel.h"

#include "Entity.h"
#include "misc/Level.h"

#include "imgui.h"

#include <algorithm>
#include <string>
#include <vector>

namespace {
constexpr const char* EntityParentDragPayloadType = "IENGINE_ENTITY_PARENT";
constexpr const char* EntityReorderDragPayloadType = "IENGINE_ENTITY_REORDER";

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
}

void EntityTreePanel::draw(
    Level& level,
    const std::string& levelLabel,
    int selectedEntityId,
    SelectEntityCallback selectEntityCallback,
    SyncEntityCallback syncEntityCallback,
    DrawEntityDetailsCallback drawEntityDetailsCallback,
    DeleteEntityCallback deleteEntityCallback,
    std::string& statusMessage
) {
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
                Entity* draggedEntity = level.findEntityById(draggedEntityId);
                if (draggedEntity != nullptr) {
                    moveEntityToRootEnd(level, *draggedEntity);
                    syncEntityCallback(*draggedEntity, true);
                    statusMessage = "Moved entity " + std::to_string(draggedEntity->getId()) + " to root order.";
                }
            } else {
                const ImGuiPayload* parentPayload = ImGui::AcceptDragDropPayload(EntityParentDragPayloadType);
                if (parentPayload != nullptr && parentPayload->IsDelivery() && parentPayload->DataSize == sizeof(int)) {
                    const int draggedEntityId = *static_cast<const int*>(parentPayload->Data);
                    Entity* draggedEntity = level.findEntityById(draggedEntityId);
                    if (draggedEntity != nullptr) {
                        draggedEntity->clearParent();
                        moveEntityToSiblingEnd(level, *draggedEntity);
                        syncEntityCallback(*draggedEntity, true);
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
                    drawEntityTreeNode(
                        level,
                        *entity,
                        selectedEntityId,
                        selectEntityCallback,
                        syncEntityCallback,
                        drawEntityDetailsCallback,
                        deleteEntityCallback,
                        statusMessage
                    );
                }
            }

            ImGui::TreePop();
        }
    }

    ImGui::EndChild();
}

void EntityTreePanel::drawEntityTreeNode(
    Level& level,
    Entity& entity,
    int selectedEntityId,
    SelectEntityCallback& selectEntityCallback,
    SyncEntityCallback& syncEntityCallback,
    DrawEntityDetailsCallback& drawEntityDetailsCallback,
    DeleteEntityCallback& deleteEntityCallback,
    std::string& statusMessage
) {
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
        selectEntityCallback(entity, false);
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
        selectEntityCallback(entity, false);
    }

    if (ImGui::BeginDragDropSource()) {
        const int entityId = entity.getId();
        ImGui::SetDragDropPayload(EntityParentDragPayloadType, &entityId, sizeof(entityId));
        ImGui::Text("Entity: %s", entity.getName().c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* reorderPayload = ImGui::AcceptDragDropPayload(EntityReorderDragPayloadType);
        if (reorderPayload != nullptr && reorderPayload->IsDelivery() && reorderPayload->DataSize == sizeof(int)) {
            const int draggedEntityId = *static_cast<const int*>(reorderPayload->Data);
            Entity* draggedEntity = level.findEntityById(draggedEntityId);
            if (draggedEntity != nullptr && draggedEntity != &entity) {
                const ImVec2 itemMin = ImGui::GetItemRectMin();
                const ImVec2 itemMax = ImGui::GetItemRectMax();
                const bool beforeTarget = ImGui::GetMousePos().y < ((itemMin.y + itemMax.y) * 0.5f);
                if (moveEntityDisplayOrder(level, *draggedEntity, entity, beforeTarget)) {
                    syncEntityCallback(*draggedEntity, true);
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
                Entity* draggedEntity = level.findEntityById(draggedEntityId);
                if (draggedEntity != nullptr && draggedEntity != &entity) {
                    if (draggedEntity->setParent(&entity)) {
                        moveEntityToSiblingEnd(level, *draggedEntity);
                        syncEntityCallback(*draggedEntity, true);
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
        drawEntityDetailsCallback(entity, statusMessage);

        std::vector<Entity*> children = getSortedChildren(entity);
        if (!children.empty()
            && ImGui::TreeNodeEx(
                "Children",
                ImGuiTreeNodeFlags_DefaultOpen |
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_SpanAvailWidth)) {
            for (Entity* child : children) {
                if (child != nullptr) {
                    drawEntityTreeNode(
                        level,
                        *child,
                        selectedEntityId,
                        selectEntityCallback,
                        syncEntityCallback,
                        drawEntityDetailsCallback,
                        deleteEntityCallback,
                        statusMessage
                    );
                }
            }

            ImGui::TreePop();
        }

        if (ImGui::SmallButton("Delete Entity")) {
            deleteEntityCallback(entity.getId());
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}
