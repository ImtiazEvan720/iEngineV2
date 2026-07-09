#include "editor/RectGizmo.h"

#include "math/Math2D.h"
#include "misc/Camera2D.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float HandleSize = 8.0f;
constexpr float HandleHalfSize = HandleSize * 0.5f;
constexpr float MinRectSize = 1.0f;

struct ScreenRect {
    Vector2F topLeft = Vector2F::zero();
    Vector2F topRight = Vector2F::zero();
    Vector2F bottomRight = Vector2F::zero();
    Vector2F bottomLeft = Vector2F::zero();
    Vector2F center = Vector2F::zero();
};

Vector2F midpoint(const Vector2F& first, const Vector2F& second) {
    return Vector2F(
        (first.x + second.x) * 0.5f,
        (first.y + second.y) * 0.5f
    );
}

Vector2F rotatedCorner(
    const RectGizmo::Rect& rect,
    float localX,
    float localY
) {
    return rect.center + Math2D::rotate(Vector2F(localX, localY), rect.rotation);
}

bool pointInRect(const Vector2F& point, const ImVec2& min, const ImVec2& max) {
    return point.x >= min.x
        && point.x <= max.x
        && point.y >= min.y
        && point.y <= max.y;
}

bool pointInHandle(const Vector2F& point, const ImVec2& center) {
    return pointInRect(
        point,
        ImVec2(center.x - HandleHalfSize, center.y - HandleHalfSize),
        ImVec2(center.x + HandleHalfSize, center.y + HandleHalfSize)
    );
}

Vector2F snapPosition(const Vector2F& position, const Vector2F& gridSize) {
    Vector2F snapped = position;

    if (gridSize.x > 0.0f) {
        const float halfGridSizeX = gridSize.x * 0.5f;
        snapped.x = std::round((position.x - halfGridSizeX) / gridSize.x) * gridSize.x + halfGridSizeX;
    }

    if (gridSize.y > 0.0f) {
        const float halfGridSizeY = gridSize.y * 0.5f;
        snapped.y = std::round((position.y - halfGridSizeY) / gridSize.y) * gridSize.y + halfGridSizeY;
    }

    return snapped;
}

bool buildScreenRect(
    const RectGizmo::Rect& rect,
    const Camera2D& camera,
    const RenderRect& viewport,
    ScreenRect& screenRect
) {
    if (rect.width <= 0.0f || rect.height <= 0.0f) {
        return false;
    }

    if (viewport.width <= 0.0f || viewport.height <= 0.0f) {
        return false;
    }

    const float halfWidth = rect.width * 0.5f;
    const float halfHeight = rect.height * 0.5f;
    screenRect.topLeft = camera.worldToScreen(
        rotatedCorner(rect, -halfWidth, -halfHeight),
        viewport
    );
    screenRect.topRight = camera.worldToScreen(
        rotatedCorner(rect, halfWidth, -halfHeight),
        viewport
    );
    screenRect.bottomRight = camera.worldToScreen(
        rotatedCorner(rect, halfWidth, halfHeight),
        viewport
    );
    screenRect.bottomLeft = camera.worldToScreen(
        rotatedCorner(rect, -halfWidth, halfHeight),
        viewport
    );
    screenRect.center = camera.worldToScreen(rect.center, viewport);
    return true;
}

bool buildScreenRectScreenSpace(
    const RectGizmo::Rect& rect,
    const RenderRect& viewport,
    ScreenRect& screenRect
) {
    if (rect.width <= 0.0f || rect.height <= 0.0f) {
        return false;
    }

    if (viewport.width <= 0.0f || viewport.height <= 0.0f) {
        return false;
    }

    const float halfWidth = rect.width * 0.5f;
    const float halfHeight = rect.height * 0.5f;
    screenRect.topLeft = rotatedCorner(rect, -halfWidth, -halfHeight);
    screenRect.topRight = rotatedCorner(rect, halfWidth, -halfHeight);
    screenRect.bottomRight = rotatedCorner(rect, halfWidth, halfHeight);
    screenRect.bottomLeft = rotatedCorner(rect, -halfWidth, halfHeight);
    screenRect.center = rect.center;
    return true;
}

bool isResizeLeftHandle(RectGizmo::Handle handle) {
    return handle == RectGizmo::Handle::ResizeLeft
        || handle == RectGizmo::Handle::ResizeTopLeft
        || handle == RectGizmo::Handle::ResizeBottomLeft;
}

bool isResizeRightHandle(RectGizmo::Handle handle) {
    return handle == RectGizmo::Handle::ResizeRight
        || handle == RectGizmo::Handle::ResizeTopRight
        || handle == RectGizmo::Handle::ResizeBottomRight;
}

bool isResizeTopHandle(RectGizmo::Handle handle) {
    return handle == RectGizmo::Handle::ResizeTop
        || handle == RectGizmo::Handle::ResizeTopLeft
        || handle == RectGizmo::Handle::ResizeTopRight;
}

bool isResizeBottomHandle(RectGizmo::Handle handle) {
    return handle == RectGizmo::Handle::ResizeBottom
        || handle == RectGizmo::Handle::ResizeBottomLeft
        || handle == RectGizmo::Handle::ResizeBottomRight;
}

bool isResizeHandle(RectGizmo::Handle handle) {
    return handle != RectGizmo::Handle::None
        && handle != RectGizmo::Handle::Move;
}

bool isHandleAllowed(RectGizmo::Handle handle, const RectGizmo::Options& options) {
    if (handle == RectGizmo::Handle::Move) {
        return options.allowMove;
    }

    if (isResizeHandle(handle)) {
        return options.allowResize;
    }

    return false;
}

RectGizmo::Rect moveRect(
    const RectGizmo::Rect& rect,
    const Vector2F& worldMousePosition,
    const Vector2F& dragOffset,
    bool snapToGrid,
    const Vector2F& gridSize
) {
    RectGizmo::Rect result = rect;
    result.center = Vector2F(
        worldMousePosition.x - dragOffset.x,
        worldMousePosition.y - dragOffset.y
    );

    if (snapToGrid) {
        result.center = snapPosition(result.center, gridSize);
    }

    return result;
}

RectGizmo::Rect resizeRect(
    const RectGizmo::Rect& rect,
    RectGizmo::Handle handle,
    const Vector2F& worldMousePosition,
    bool snapToGrid,
    const Vector2F& gridSize
) {
    RectGizmo::Rect result = rect;

    float left = rect.width * -0.5f;
    float right = rect.width * 0.5f;
    float top = rect.height * -0.5f;
    float bottom = rect.height * 0.5f;
    Vector2F snappedMousePosition = worldMousePosition;

    if (snapToGrid) {
        snappedMousePosition = snapPosition(worldMousePosition, gridSize);
    }

    const Vector2F localMousePosition =
        Math2D::inverseRotate(snappedMousePosition - rect.center, rect.rotation);

    if (isResizeLeftHandle(handle)) {
        left = std::min(localMousePosition.x, right - MinRectSize);
    } else if (isResizeRightHandle(handle)) {
        right = std::max(localMousePosition.x, left + MinRectSize);
    }

    if (isResizeTopHandle(handle)) {
        top = std::min(localMousePosition.y, bottom - MinRectSize);
    } else if (isResizeBottomHandle(handle)) {
        bottom = std::max(localMousePosition.y, top + MinRectSize);
    }

    result.width = std::max(MinRectSize, right - left);
    result.height = std::max(MinRectSize, bottom - top);
    const Vector2F localCenterOffset(
        left + (result.width * 0.5f),
        top + (result.height * 0.5f)
    );
    result.center = rect.center + Math2D::rotate(localCenterOffset, rect.rotation);

    return result;
}

void addHandle(
    ImDrawList& drawList,
    const ImVec2& center,
    ImU32 fillColor,
    ImU32 outlineColor
) {
    drawList.AddRectFilled(
        ImVec2(center.x - HandleHalfSize, center.y - HandleHalfSize),
        ImVec2(center.x + HandleHalfSize, center.y + HandleHalfSize),
        fillColor
    );
    drawList.AddRect(
        ImVec2(center.x - HandleHalfSize, center.y - HandleHalfSize),
        ImVec2(center.x + HandleHalfSize, center.y + HandleHalfSize),
        outlineColor
    );
}

void addRectHandles(
    ImDrawList& drawList,
    const ScreenRect& screenRect,
    bool drawCenter,
    ImU32 fillColor,
    ImU32 centerFillColor,
    ImU32 outlineColor
) {
    const Vector2F topCenter = midpoint(screenRect.topLeft, screenRect.topRight);
    const Vector2F rightCenter = midpoint(screenRect.topRight, screenRect.bottomRight);
    const Vector2F bottomCenter = midpoint(screenRect.bottomLeft, screenRect.bottomRight);
    const Vector2F leftCenter = midpoint(screenRect.topLeft, screenRect.bottomLeft);

    addHandle(drawList, ImVec2(screenRect.topLeft.x, screenRect.topLeft.y), fillColor, outlineColor);
    addHandle(drawList, ImVec2(topCenter.x, topCenter.y), fillColor, outlineColor);
    addHandle(drawList, ImVec2(screenRect.topRight.x, screenRect.topRight.y), fillColor, outlineColor);

    addHandle(drawList, ImVec2(leftCenter.x, leftCenter.y), fillColor, outlineColor);
    addHandle(drawList, ImVec2(rightCenter.x, rightCenter.y), fillColor, outlineColor);

    addHandle(drawList, ImVec2(screenRect.bottomLeft.x, screenRect.bottomLeft.y), fillColor, outlineColor);
    addHandle(drawList, ImVec2(bottomCenter.x, bottomCenter.y), fillColor, outlineColor);
    addHandle(drawList, ImVec2(screenRect.bottomRight.x, screenRect.bottomRight.y), fillColor, outlineColor);
    if (drawCenter) {
        addHandle(
            drawList,
            ImVec2(screenRect.center.x, screenRect.center.y),
            centerFillColor,
            outlineColor
        );
    }
}

void drawHandlesFromScreenRect(
    const ScreenRect& screenRect,
    const RenderRect& viewport,
    bool showCenter
) {
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

    const ImVec2 shadowOffset(1.0f, 1.0f);
    const ImVec2 topLeft(screenRect.topLeft.x, screenRect.topLeft.y);
    const ImVec2 topRight(screenRect.topRight.x, screenRect.topRight.y);
    const ImVec2 bottomRight(screenRect.bottomRight.x, screenRect.bottomRight.y);
    const ImVec2 bottomLeft(screenRect.bottomLeft.x, screenRect.bottomLeft.y);

    drawList->AddQuad(
        ImVec2(topLeft.x + shadowOffset.x, topLeft.y + shadowOffset.y),
        ImVec2(topRight.x + shadowOffset.x, topRight.y + shadowOffset.y),
        ImVec2(bottomRight.x + shadowOffset.x, bottomRight.y + shadowOffset.y),
        ImVec2(bottomLeft.x + shadowOffset.x, bottomLeft.y + shadowOffset.y),
        shadowColor,
        1.0f
    );

    drawList->AddQuad(
        topLeft,
        topRight,
        bottomRight,
        bottomLeft,
        outlineColor,
        2.0f
    );

    addRectHandles(
        *drawList,
        screenRect,
        showCenter,
        fillColor,
        centerFillColor,
        outlineColor
    );

    drawList->PopClipRect();
}

RectGizmo::Handle hitTestScreenRect(
    const ScreenRect& screenRect,
    const Vector2F& screenMousePosition,
    bool showCenterHandle
) {
    if (showCenterHandle) {
        if (pointInHandle(
                screenMousePosition,
                ImVec2(screenRect.center.x, screenRect.center.y))) {
            return RectGizmo::Handle::Move;
        }
    }

    if (pointInHandle(
            screenMousePosition,
            ImVec2(screenRect.topLeft.x, screenRect.topLeft.y))) {
        return RectGizmo::Handle::ResizeTopLeft;
    }

    if (pointInHandle(
            screenMousePosition,
            ImVec2(screenRect.topRight.x, screenRect.topRight.y))) {
        return RectGizmo::Handle::ResizeTopRight;
    }

    if (pointInHandle(
            screenMousePosition,
            ImVec2(screenRect.bottomLeft.x, screenRect.bottomLeft.y))) {
        return RectGizmo::Handle::ResizeBottomLeft;
    }

    if (pointInHandle(
            screenMousePosition,
            ImVec2(screenRect.bottomRight.x, screenRect.bottomRight.y))) {
        return RectGizmo::Handle::ResizeBottomRight;
    }

    const Vector2F topCenter = midpoint(screenRect.topLeft, screenRect.topRight);
    const Vector2F rightCenter = midpoint(screenRect.topRight, screenRect.bottomRight);
    const Vector2F bottomCenter = midpoint(screenRect.bottomLeft, screenRect.bottomRight);
    const Vector2F leftCenter = midpoint(screenRect.topLeft, screenRect.bottomLeft);

    if (pointInHandle(screenMousePosition, ImVec2(topCenter.x, topCenter.y))) {
        return RectGizmo::Handle::ResizeTop;
    }

    if (pointInHandle(screenMousePosition, ImVec2(bottomCenter.x, bottomCenter.y))) {
        return RectGizmo::Handle::ResizeBottom;
    }

    if (pointInHandle(screenMousePosition, ImVec2(leftCenter.x, leftCenter.y))) {
        return RectGizmo::Handle::ResizeLeft;
    }

    if (pointInHandle(screenMousePosition, ImVec2(rightCenter.x, rightCenter.y))) {
        return RectGizmo::Handle::ResizeRight;
    }

    return RectGizmo::Handle::None;
}

RectGizmo::EditResult handleRectInteraction(
    int id,
    const RectGizmo::Rect& rect,
    RectGizmo::Handle hitHandle,
    const Vector2F& mousePosition,
    bool snapToGrid,
    const Vector2F& gridSize,
    const RectGizmo::Options& options,
    int& editingId,
    RectGizmo::Handle& activeHandle,
    Vector2F& dragOffset
) {
    RectGizmo::EditResult result;
    result.rect = rect;
    result.handle = activeHandle;

    const ImGuiIO& io = ImGui::GetIO();

    if (activeHandle != RectGizmo::Handle::None) {
        if (editingId != id) {
            editingId = -1;
            activeHandle = RectGizmo::Handle::None;
            dragOffset = Vector2F::zero();
            return result;
        }

        if (!isHandleAllowed(activeHandle, options)) {
            editingId = -1;
            activeHandle = RectGizmo::Handle::None;
            dragOffset = Vector2F::zero();
            return result;
        }

        result.active = true;
        result.handle = activeHandle;

        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (activeHandle == RectGizmo::Handle::Move) {
                result.rect = moveRect(
                    rect,
                    mousePosition,
                    dragOffset,
                    snapToGrid,
                    gridSize
                );
            } else {
                result.rect = resizeRect(
                    rect,
                    activeHandle,
                    mousePosition,
                    snapToGrid,
                    gridSize
                );
            }

            result.changed = true;
            return result;
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            result.finished = true;
            editingId = -1;
            activeHandle = RectGizmo::Handle::None;
            dragOffset = Vector2F::zero();
            return result;
        }

        return result;
    }

    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left) || io.WantCaptureMouse) {
        return result;
    }

    if (!isHandleAllowed(hitHandle, options)) {
        return result;
    }

    editingId = id;
    activeHandle = hitHandle;
    result.active = true;
    result.handle = hitHandle;

    if (hitHandle == RectGizmo::Handle::Move) {
        dragOffset = Vector2F(
            mousePosition.x - rect.center.x,
            mousePosition.y - rect.center.y
        );
    } else {
        dragOffset = Vector2F::zero();
    }

    return result;
}
}

void RectGizmo::drawHandles(
    const Rect& rect,
    const Camera2D& camera,
    const RenderRect& viewport
) const {
    ScreenRect screenRect;
    if (!buildScreenRect(rect, camera, viewport, screenRect)) {
        return;
    }

    drawHandlesFromScreenRect(screenRect, viewport,true);
}

void RectGizmo::drawHandlesScreenSpace(
    const Rect& rect,
    const RenderRect& viewport
) const {
    ScreenRect screenRect;
    if (!buildScreenRectScreenSpace(rect, viewport, screenRect)) {
        return;
    }

    drawHandlesFromScreenRect(screenRect, viewport,false);
}

RectGizmo::Handle RectGizmo::hitTest(
    const Rect& rect,
    const Camera2D& camera,
    const RenderRect& viewport,
    const Vector2F& screenMousePosition
) const {
    ScreenRect screenRect;
    if (!buildScreenRect(rect, camera, viewport, screenRect)) {
        return Handle::None;
    }

    return hitTestScreenRect(screenRect, screenMousePosition,true);
}

RectGizmo::Handle RectGizmo::hitTestScreenSpace(
    const Rect& rect,
    const RenderRect& viewport,
    const Vector2F& screenMousePosition
) const {
    ScreenRect screenRect;
    if (!buildScreenRectScreenSpace(rect, viewport, screenRect)) {
        return Handle::None;
    }

    return hitTestScreenRect(screenRect, screenMousePosition,false);
}

void RectGizmo::cancel() {
    editingId = -1;
    activeHandle = Handle::None;
    dragOffset = Vector2F::zero();
}

RectGizmo::EditResult RectGizmo::handleInteraction(
    int id,
    const Rect& rect,
    const Camera2D& camera,
    const RenderRect& viewport,
    const Vector2F& worldMousePosition,
    bool snapToGrid,
    const Vector2F& gridSize,
    const Options& options
) {
    const ImVec2 mousePosition = ImGui::GetMousePos();
    const Vector2F screenMousePosition(mousePosition.x, mousePosition.y);

    const Handle hitHandle = hitTest(rect, camera, viewport, screenMousePosition);
    return handleRectInteraction(
        id,
        rect,
        hitHandle,
        worldMousePosition,
        snapToGrid,
        gridSize,
        options,
        editingId,
        activeHandle,
        dragOffset
    );
}

RectGizmo::EditResult RectGizmo::handleInteractionScreenSpace(
    int id,
    const Rect& rect,
    const RenderRect& viewport,
    const Vector2F& screenMousePosition,
    bool snapToGrid,
    const Vector2F& gridSize,
    const Options& options
) {
    const Handle hitHandle = hitTestScreenSpace(rect, viewport, screenMousePosition);
    return handleRectInteraction(
        id,
        rect,
        hitHandle,
        screenMousePosition,
        snapToGrid,
        gridSize,
        options,
        editingId,
        activeHandle,
        dragOffset
    );
}
