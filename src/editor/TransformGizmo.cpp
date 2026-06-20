#include "editor/TransformGizmo.h"

#include "Entity.h"
#include "components/TransformComponent.h"
#include "editor/EditorCamera.h"
#include "editor/EntityInspectorPanel.h"
#include "math/Vector2F.h"
#include "misc/Camera2D.h"
#include "misc/Level.h"
#include "system/IRenderBackend.h"
#include "system/Renderer.h"

#include "imgui.h"

namespace {
constexpr float TransformHandleRadius = 6.0f;
constexpr float TransformHandleHitRadius = 10.0f;
const float TransformHandleLineLength = 15.0f;
const float TransformHandleArrowSize = 2.5f;

bool pointInCircle(const Vector2F& point, const ImVec2& center, float radius) {
    const float deltaX = point.x - center.x;
    const float deltaY = point.y - center.y;
    return (deltaX * deltaX) + (deltaY * deltaY) <= radius * radius;
}

bool pointInViewport(const Vector2F& point, const RenderRect& viewport, float margin = 0.0f) {
    return point.x >= viewport.x - margin
        && point.x <= viewport.x + viewport.width + margin
        && point.y >= viewport.y - margin
        && point.y <= viewport.y + viewport.height + margin;
}

void addTransformHandle(
    ImDrawList& drawList,
    const ImVec2& center,
    bool selected,
    bool disabled
) {
    const ImU32 shadowColor = IM_COL32(0, 0, 0, 150);
    const ImU32 outlineColor = selected
        ? IM_COL32(255, 220, 70, 255)
        : IM_COL32(80, 210, 255, disabled ? 150 : 235);
    const ImU32 xAxisColor = disabled
        ? IM_COL32(100, 130, 100, 140)
        : IM_COL32(70, 220, 90, 255);
    const ImU32 yAxisColor = disabled
        ? IM_COL32(140, 90, 90, 140)
        : IM_COL32(235, 70, 70, 255);
    const ImU32 selectedFillColor = disabled
        ? IM_COL32(120, 120, 120, 90)
        : IM_COL32(255, 220, 70, 230);

    drawList.AddCircleFilled(center, TransformHandleRadius + 2.0f, shadowColor);
    drawList.AddLine(
        ImVec2(center.x, center.y),
        ImVec2(center.x + TransformHandleLineLength, center.y),
        xAxisColor,
        1.5f
    );
    drawList.AddTriangleFilled(
        ImVec2(center.x + TransformHandleLineLength + TransformHandleArrowSize, center.y),
        ImVec2(center.x + TransformHandleLineLength, center.y - TransformHandleArrowSize),
        ImVec2(center.x + TransformHandleLineLength, center.y + TransformHandleArrowSize),
        xAxisColor
    );
    drawList.AddLine(
        ImVec2(center.x, center.y),
        ImVec2(center.x, center.y - TransformHandleLineLength),
        yAxisColor,
        1.5f
    );
    drawList.AddTriangleFilled(
        ImVec2(center.x, center.y - TransformHandleLineLength - TransformHandleArrowSize),
        ImVec2(center.x - TransformHandleArrowSize, center.y - TransformHandleLineLength),
        ImVec2(center.x + TransformHandleArrowSize, center.y - TransformHandleLineLength),
        yAxisColor
    );
    if (selected) {
        drawList.AddCircleFilled(center, TransformHandleRadius, selectedFillColor);
        drawList.AddCircle(center, TransformHandleRadius, outlineColor, 16, 2.5f);
    }
}
}

void TransformGizmo::draw(
    Level& level,
    const EntityInspectorPanel& entityInspector,
    const EditorCamera& camera
) const {
    const RenderRect viewport = camera.getViewport();
    if (viewport.width <= 0.0f || viewport.height <= 0.0f) {
        return;
    }

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    if (drawList == nullptr) {
        return;
    }

    const Camera2D& sceneCamera = Renderer::getInstance().getCamera();
    const ImVec2 clipMin(viewport.x, viewport.y);
    const ImVec2 clipMax(viewport.x + viewport.width, viewport.y + viewport.height);
    const int selectedEntityId = entityInspector.getSelectedEntityId();

    drawList->PushClipRect(clipMin, clipMax, true);

    for (Entity& entity : level.getEntities()) {
        if (entity.isDestroyed()) {
            continue;
        }

        TransformComponent* transform = entity.getComponent<TransformComponent>();
        if (transform == nullptr) {
            continue;
        }

        const Vector2F screenPosition = sceneCamera.worldToScreen(
            transform->getWorldPosition(),
            viewport
        );

        if (!pointInViewport(screenPosition, viewport, TransformHandleHitRadius)) {
            continue;
        }

        addTransformHandle(
            *drawList,
            ImVec2(screenPosition.x, screenPosition.y),
            entity.getId() == selectedEntityId,
            !entity.isEnabled()
        );
    }

    drawList->PopClipRect();
}

bool TransformGizmo::contains(
    Entity& entity,
    float worldX,
    float worldY,
    const EditorCamera& camera
) const {
    TransformComponent* transform = entity.getComponent<TransformComponent>();
    if (transform == nullptr) {
        return false;
    }

    const RenderRect viewport = camera.getViewport();
    const Camera2D& sceneCamera = Renderer::getInstance().getCamera();
    const Vector2F screenMousePosition = sceneCamera.worldToScreen(Vector2F(worldX, worldY), viewport);
    const Vector2F screenTransformPosition = sceneCamera.worldToScreen(
        transform->getWorldPosition(),
        viewport
    );

    return pointInViewport(screenTransformPosition, viewport, TransformHandleHitRadius)
        && pointInCircle(
            screenMousePosition,
            ImVec2(screenTransformPosition.x, screenTransformPosition.y),
            TransformHandleHitRadius
        );
}
