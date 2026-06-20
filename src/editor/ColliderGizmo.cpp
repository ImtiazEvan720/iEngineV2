#include "editor/ColliderGizmo.h"

#include "Entity.h"
#include "components/CollisionComponent.h"
#include "components/TransformComponent.h"
#include "editor/EditorCamera.h"
#include "editor/EntityInspectorPanel.h"
#include "editor/ViewportGrid.h"
#include "misc/Camera2D.h"
#include "misc/Level.h"
#include "system/Renderer.h"

#include "imgui.h"

#include <algorithm>

namespace {
constexpr float ColliderHandleSize = 8.0f;
constexpr float ColliderMinSize = 1.0f;

bool pointInRect(const Vector2F& point, const ImVec2& min, const ImVec2& max) {
    return point.x >= min.x
        && point.x <= max.x
        && point.y >= min.y
        && point.y <= max.y;
}

bool pointInHandle(const Vector2F& point, const ImVec2& center) {
    const float halfSize = ColliderHandleSize * 0.5f;
    return pointInRect(
        point,
        ImVec2(center.x - halfSize, center.y - halfSize),
        ImVec2(center.x + halfSize, center.y + halfSize)
    );
}

void addColliderHandle(ImDrawList& drawList, const ImVec2& center, ImU32 fillColor, ImU32 outlineColor) {
    const float halfSize = ColliderHandleSize * 0.5f;
    drawList.AddRectFilled(
        ImVec2(center.x - halfSize, center.y - halfSize),
        ImVec2(center.x + halfSize, center.y + halfSize),
        fillColor
    );
    drawList.AddRect(
        ImVec2(center.x - halfSize, center.y - halfSize),
        ImVec2(center.x + halfSize, center.y + halfSize),
        outlineColor
    );
}
}

void ColliderGizmo::cancel() {
    editingEntityId = -1;
    editMode = EditMode::None;
}

void ColliderGizmo::draw(
    const EntityInspectorPanel& entityInspector,
    const EditorCamera& camera
) const {
    const int selectedEntityId = entityInspector.getSelectedEntityId();
    if (selectedEntityId < 0) {
        return;
    }

    Entity* entity = findEntityById(selectedEntityId);
    if (entity == nullptr) {
        return;
    }

    TransformComponent* transform = entity->getComponent<TransformComponent>();
    CollisionComponent* collider = entity->getComponent<CollisionComponent>();
    if (transform == nullptr || collider == nullptr) {
        return;
    }

    const RenderRect viewport = camera.getViewport();
    if (viewport.width <= 0.0f || viewport.height <= 0.0f) {
        return;
    }

    const Camera2D& sceneCamera = Renderer::getInstance().getCamera();
    Vector2F worldCenter = transform->getWorldPosition();
    const Vector2F& offset = collider->getOffset();
    worldCenter.x += offset.x;
    worldCenter.y += offset.y;
    const float halfWidth = collider->getWidth() * 0.5f;
    const float halfHeight = collider->getHeight() * 0.5f;

    const Vector2F screenTopLeft = sceneCamera.worldToScreen(
        Vector2F(worldCenter.x - halfWidth, worldCenter.y - halfHeight),
        viewport
    );
    const Vector2F screenBottomRight = sceneCamera.worldToScreen(
        Vector2F(worldCenter.x + halfWidth, worldCenter.y + halfHeight),
        viewport
    );

    const float left = std::min(screenTopLeft.x, screenBottomRight.x);
    const float right = std::max(screenTopLeft.x, screenBottomRight.x);
    const float top = std::min(screenTopLeft.y, screenBottomRight.y);
    const float bottom = std::max(screenTopLeft.y, screenBottomRight.y);
    const float centerX = (left + right) * 0.5f;
    const float centerY = (top + bottom) * 0.5f;

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    if (drawList == nullptr) {
        return;
    }

    const ImVec2 clipMin(viewport.x, viewport.y);
    const ImVec2 clipMax(viewport.x + viewport.width, viewport.y + viewport.height);
    const ImU32 outlineColor = IM_COL32(70, 210, 255, 235);
    const ImU32 fillColor = IM_COL32(70, 210, 255, 180);
    const ImU32 centerFillColor = IM_COL32(255, 220, 70, 210);
    const ImU32 shadowColor = IM_COL32(0, 0, 0, 130);

    drawList->PushClipRect(clipMin, clipMax, true);
    drawList->AddRect(ImVec2(left + 1.0f, top + 1.0f), ImVec2(right + 1.0f, bottom + 1.0f), shadowColor);
    drawList->AddRect(ImVec2(left, top), ImVec2(right, bottom), outlineColor, 0.0f, 0, 2.0f);

    addColliderHandle(*drawList, ImVec2(left, top), fillColor, outlineColor);
    addColliderHandle(*drawList, ImVec2(centerX, top), fillColor, outlineColor);
    addColliderHandle(*drawList, ImVec2(right, top), fillColor, outlineColor);
    addColliderHandle(*drawList, ImVec2(left, centerY), fillColor, outlineColor);
    addColliderHandle(*drawList, ImVec2(right, centerY), fillColor, outlineColor);
    addColliderHandle(*drawList, ImVec2(left, bottom), fillColor, outlineColor);
    addColliderHandle(*drawList, ImVec2(centerX, bottom), fillColor, outlineColor);
    addColliderHandle(*drawList, ImVec2(right, bottom), fillColor, outlineColor);
    addColliderHandle(*drawList, ImVec2(centerX, centerY), centerFillColor, outlineColor);
    drawList->PopClipRect();
}

bool ColliderGizmo::handleInteraction(
    EntityInspectorPanel& entityInspector,
    const ViewportGrid& viewportGrid,
    const EditorCamera& camera,
    const Vector2F& worldMousePosition,
    const RenderRect& viewport,
    int& draggingEntityId,
    std::string& statusMessage
) {
    (void)camera;

    if (!viewportGrid.shouldShowColliders()) {
        cancel();
        return false;
    }

    const ImGuiIO& io = ImGui::GetIO();
    const ImVec2 mousePosition = ImGui::GetMousePos();
    const Vector2F screenMousePosition(mousePosition.x, mousePosition.y);

    if (editMode != EditMode::None) {
        if (editingEntityId < 0 || findEntityById(editingEntityId) == nullptr) {
            cancel();
            return false;
        }

        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (editMode == EditMode::Move) {
                moveSelectedCollider(entityInspector, viewportGrid, worldMousePosition);
            } else {
                resizeSelectedCollider(entityInspector, viewportGrid, worldMousePosition);
            }

            return true;
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            Entity* entity = findEntityById(editingEntityId);
            if (entity != nullptr) {
                entityInspector.syncEditStateFromEntity(*entity, true);
                if (editMode == EditMode::Move) {
                    statusMessage = "Moved collider offset for entity "
                        + std::to_string(entity->getId()) + ".";
                } else {
                    statusMessage = "Updated collider bounds for entity "
                        + std::to_string(entity->getId()) + ".";
                }
            }

            cancel();
            return true;
        }

        return true;
    }

    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left) || io.WantCaptureMouse) {
        return false;
    }

    const EditMode hitMode = hitTestSelectedHandle(entityInspector, screenMousePosition, viewport);
    if (hitMode == EditMode::None) {
        return false;
    }

    editingEntityId = entityInspector.getSelectedEntityId();
    editMode = hitMode;
    draggingEntityId = -1;

    if (Entity* entity = findEntityById(editingEntityId)) {
        if (TransformComponent* transform = entity->getComponent<TransformComponent>()) {
            if (CollisionComponent* collider = entity->getComponent<CollisionComponent>()) {
                Vector2F colliderWorldCenter = transform->getWorldPosition();
                const Vector2F& offset = collider->getOffset();
                colliderWorldCenter.x += offset.x;
                colliderWorldCenter.y += offset.y;
                dragOffset[0] = worldMousePosition.x - colliderWorldCenter.x;
                dragOffset[1] = worldMousePosition.y - colliderWorldCenter.y;
            }
        }
    }

    statusMessage = hitMode == EditMode::Move
        ? "Moving collider."
        : "Editing collider bounds.";
    return true;
}

ColliderGizmo::EditMode ColliderGizmo::hitTestSelectedHandle(
    const EntityInspectorPanel& entityInspector,
    const Vector2F& screenMousePosition,
    const RenderRect& viewport
) const {
    const int selectedEntityId = entityInspector.getSelectedEntityId();
    if (selectedEntityId < 0) {
        return EditMode::None;
    }

    Entity* entity = findEntityById(selectedEntityId);
    if (entity == nullptr) {
        return EditMode::None;
    }

    TransformComponent* transform = entity->getComponent<TransformComponent>();
    CollisionComponent* collider = entity->getComponent<CollisionComponent>();
    if (transform == nullptr || collider == nullptr) {
        return EditMode::None;
    }

    const Camera2D& sceneCamera = Renderer::getInstance().getCamera();
    Vector2F worldCenter = transform->getWorldPosition();
    const Vector2F& offset = collider->getOffset();
    worldCenter.x += offset.x;
    worldCenter.y += offset.y;
    const float halfWidth = collider->getWidth() * 0.5f;
    const float halfHeight = collider->getHeight() * 0.5f;

    const Vector2F screenTopLeft = sceneCamera.worldToScreen(
        Vector2F(worldCenter.x - halfWidth, worldCenter.y - halfHeight),
        viewport
    );
    const Vector2F screenBottomRight = sceneCamera.worldToScreen(
        Vector2F(worldCenter.x + halfWidth, worldCenter.y + halfHeight),
        viewport
    );

    const float left = std::min(screenTopLeft.x, screenBottomRight.x);
    const float right = std::max(screenTopLeft.x, screenBottomRight.x);
    const float top = std::min(screenTopLeft.y, screenBottomRight.y);
    const float bottom = std::max(screenTopLeft.y, screenBottomRight.y);
    const float centerX = (left + right) * 0.5f;
    const float centerY = (top + bottom) * 0.5f;

    if (pointInHandle(screenMousePosition, ImVec2(centerX, centerY))) {
        return EditMode::Move;
    }

    if (pointInHandle(screenMousePosition, ImVec2(left, top))) {
        return EditMode::ResizeTopLeft;
    }

    if (pointInHandle(screenMousePosition, ImVec2(right, top))) {
        return EditMode::ResizeTopRight;
    }

    if (pointInHandle(screenMousePosition, ImVec2(left, bottom))) {
        return EditMode::ResizeBottomLeft;
    }

    if (pointInHandle(screenMousePosition, ImVec2(right, bottom))) {
        return EditMode::ResizeBottomRight;
    }

    if (pointInHandle(screenMousePosition, ImVec2(centerX, top))) {
        return EditMode::ResizeTop;
    }

    if (pointInHandle(screenMousePosition, ImVec2(centerX, bottom))) {
        return EditMode::ResizeBottom;
    }

    if (pointInHandle(screenMousePosition, ImVec2(left, centerY))) {
        return EditMode::ResizeLeft;
    }

    if (pointInHandle(screenMousePosition, ImVec2(right, centerY))) {
        return EditMode::ResizeRight;
    }

    return EditMode::None;
}

void ColliderGizmo::moveSelectedCollider(
    EntityInspectorPanel& entityInspector,
    const ViewportGrid& viewportGrid,
    const Vector2F& worldMousePosition
) {
    Entity* entity = findEntityById(editingEntityId);
    if (entity == nullptr) {
        return;
    }

    TransformComponent* transform = entity->getComponent<TransformComponent>();
    CollisionComponent* collider = entity->getComponent<CollisionComponent>();
    if (transform == nullptr || collider == nullptr) {
        return;
    }

    Vector2F colliderWorldCenter(
        worldMousePosition.x - dragOffset[0],
        worldMousePosition.y - dragOffset[1]
    );

    if (viewportGrid.shouldSnapToGrid()) {
        colliderWorldCenter = viewportGrid.snapPosition(colliderWorldCenter);
    }

    const Vector2F transformWorldPosition = transform->getWorldPosition();
    collider->setOffset(Vector2F(
        colliderWorldCenter.x - transformWorldPosition.x,
        colliderWorldCenter.y - transformWorldPosition.y
    ));
    entityInspector.syncEditStateFromEntity(*entity, true);
}

void ColliderGizmo::resizeSelectedCollider(
    EntityInspectorPanel& entityInspector,
    const ViewportGrid& viewportGrid,
    const Vector2F& worldMousePosition
) {
    Entity* entity = findEntityById(editingEntityId);
    if (entity == nullptr) {
        return;
    }

    TransformComponent* transform = entity->getComponent<TransformComponent>();
    CollisionComponent* collider = entity->getComponent<CollisionComponent>();
    if (transform == nullptr || collider == nullptr) {
        return;
    }

    const Vector2F transformWorldPosition = transform->getWorldPosition();
    Vector2F worldCenter = transformWorldPosition;
    const Vector2F& offset = collider->getOffset();
    worldCenter.x += offset.x;
    worldCenter.y += offset.y;

    float left = worldCenter.x - (collider->getWidth() * 0.5f);
    float right = worldCenter.x + (collider->getWidth() * 0.5f);
    float top = worldCenter.y - (collider->getHeight() * 0.5f);
    float bottom = worldCenter.y + (collider->getHeight() * 0.5f);
    Vector2F snappedMousePosition = worldMousePosition;

    if (viewportGrid.shouldSnapToGrid()) {
        snappedMousePosition = viewportGrid.snapPosition(worldMousePosition);
    }

    switch (editMode) {
        case EditMode::ResizeLeft:
        case EditMode::ResizeTopLeft:
        case EditMode::ResizeBottomLeft:
            left = std::min(snappedMousePosition.x, right - ColliderMinSize);
            break;
        case EditMode::ResizeRight:
        case EditMode::ResizeTopRight:
        case EditMode::ResizeBottomRight:
            right = std::max(snappedMousePosition.x, left + ColliderMinSize);
            break;
        case EditMode::ResizeTop:
        case EditMode::ResizeBottom:
        case EditMode::Move:
        case EditMode::None:
            break;
    }

    switch (editMode) {
        case EditMode::ResizeTop:
        case EditMode::ResizeTopLeft:
        case EditMode::ResizeTopRight:
            top = std::min(snappedMousePosition.y, bottom - ColliderMinSize);
            break;
        case EditMode::ResizeBottom:
        case EditMode::ResizeBottomLeft:
        case EditMode::ResizeBottomRight:
            bottom = std::max(snappedMousePosition.y, top + ColliderMinSize);
            break;
        case EditMode::ResizeLeft:
        case EditMode::ResizeRight:
        case EditMode::Move:
        case EditMode::None:
            break;
    }

    const float width = std::max(ColliderMinSize, right - left);
    const float height = std::max(ColliderMinSize, bottom - top);
    const Vector2F newCenter(
        left + (width * 0.5f),
        top + (height * 0.5f)
    );

    collider->setSize(width, height);
    collider->setOffset(Vector2F(
        newCenter.x - transformWorldPosition.x,
        newCenter.y - transformWorldPosition.y
    ));
    entityInspector.syncEditStateFromEntity(*entity, true);
}

Entity* ColliderGizmo::findEntityById(int id) const {
    for (Entity& entity : Level::getCurrentLevel().getEntities()) {
        if (entity.getId() == id && !entity.isDestroyed()) {
            return &entity;
        }
    }

    return nullptr;
}
