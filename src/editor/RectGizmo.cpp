#include "editor/RectGizmo.h"

#include "misc/Camera2D.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float HandleSize = 8.0f;
constexpr float HandleHalfSize = HandleSize * 0.5f;
constexpr float MinRectSize = 1.0f;

struct ScreenRect {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    float centerX() const {
        return (left + right) * 0.5f;
    }

    float centerY() const {
        return (top + bottom) * 0.5f;
    }
};

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

    const Vector2F halfSize(rect.width * 0.5f, rect.height * 0.5f);

    const Vector2F screenTopLeft = camera.worldToScreen(
        Vector2F(rect.center.x - halfSize.x, rect.center.y - halfSize.y),
        viewport
    );

    const Vector2F screenBottomRight = camera.worldToScreen(
        Vector2F(rect.center.x + halfSize.x, rect.center.y + halfSize.y),
        viewport
    );

    screenRect.left = std::min(screenTopLeft.x, screenBottomRight.x);
    screenRect.right = std::max(screenTopLeft.x, screenBottomRight.x);
    screenRect.top = std::min(screenTopLeft.y, screenBottomRight.y);
    screenRect.bottom = std::max(screenTopLeft.y, screenBottomRight.y);
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

    const Vector2F halfSize(rect.width * 0.5f, rect.height * 0.5f);
    screenRect.left = rect.center.x - halfSize.x;
    screenRect.right = rect.center.x + halfSize.x;
    screenRect.top = rect.center.y - halfSize.y;
    screenRect.bottom = rect.center.y + halfSize.y;
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

    float left = rect.center.x - (rect.width * 0.5f);
    float right = rect.center.x + (rect.width * 0.5f);
    float top = rect.center.y - (rect.height * 0.5f);
    float bottom = rect.center.y + (rect.height * 0.5f);
    Vector2F snappedMousePosition = worldMousePosition;

    if (snapToGrid) {
        snappedMousePosition = snapPosition(worldMousePosition, gridSize);
    }

    if (isResizeLeftHandle(handle)) {
        left = std::min(snappedMousePosition.x, right - MinRectSize);
    } else if (isResizeRightHandle(handle)) {
        right = std::max(snappedMousePosition.x, left + MinRectSize);
    }

    if (isResizeTopHandle(handle)) {
        top = std::min(snappedMousePosition.y, bottom - MinRectSize);
    } else if (isResizeBottomHandle(handle)) {
        bottom = std::max(snappedMousePosition.y, top + MinRectSize);
    }

    result.width = std::max(MinRectSize, right - left);
    result.height = std::max(MinRectSize, bottom - top);
    result.center = Vector2F(
        left + (result.width * 0.5f),
        top + (result.height * 0.5f)
    );

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
    float left,
    float top,
    float right,
    float bottom,
    bool drawCenter,
    ImU32 fillColor,
    ImU32 centerFillColor,
    ImU32 outlineColor
) {
    const float centerX = (left + right) * 0.5f;
    const float centerY = (top + bottom) * 0.5f;

    addHandle(drawList, ImVec2(left, top), fillColor, outlineColor);
    addHandle(drawList, ImVec2(centerX, top), fillColor, outlineColor);
    addHandle(drawList, ImVec2(right, top), fillColor, outlineColor);

    addHandle(drawList, ImVec2(left, centerY), fillColor, outlineColor);
    addHandle(drawList, ImVec2(right, centerY), fillColor, outlineColor);

    addHandle(drawList, ImVec2(left, bottom), fillColor, outlineColor);
    addHandle(drawList, ImVec2(centerX, bottom), fillColor, outlineColor);
    addHandle(drawList, ImVec2(right, bottom), fillColor, outlineColor);
    if (drawCenter) {
        addHandle(drawList, ImVec2(centerX, centerY), centerFillColor, outlineColor);
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

    drawList->AddRect(
        ImVec2(screenRect.left + 1.0f, screenRect.top + 1.0f),
        ImVec2(screenRect.right + 1.0f, screenRect.bottom + 1.0f),
        shadowColor
    );

    drawList->AddRect(
        ImVec2(screenRect.left, screenRect.top),
        ImVec2(screenRect.right, screenRect.bottom),
        outlineColor,
        0.0f,
        0,
        2.0f
    );

    addRectHandles(
        *drawList,
        screenRect.left,
        screenRect.top,
        screenRect.right,
        screenRect.bottom,
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
    if(showCenterHandle)
    {
        if (pointInHandle(screenMousePosition, ImVec2(screenRect.centerX(), screenRect.centerY()))) {
           return RectGizmo::Handle::Move;
        }
    }

    if (pointInHandle(screenMousePosition, ImVec2(screenRect.left, screenRect.top))) {
        return RectGizmo::Handle::ResizeTopLeft;
    }

    if (pointInHandle(screenMousePosition, ImVec2(screenRect.right, screenRect.top))) {
        return RectGizmo::Handle::ResizeTopRight;
    }

    if (pointInHandle(screenMousePosition, ImVec2(screenRect.left, screenRect.bottom))) {
        return RectGizmo::Handle::ResizeBottomLeft;
    }

    if (pointInHandle(screenMousePosition, ImVec2(screenRect.right, screenRect.bottom))) {
        return RectGizmo::Handle::ResizeBottomRight;
    }

    if (pointInHandle(screenMousePosition, ImVec2(screenRect.centerX(), screenRect.top))) {
        return RectGizmo::Handle::ResizeTop;
    }

    if (pointInHandle(screenMousePosition, ImVec2(screenRect.centerX(), screenRect.bottom))) {
        return RectGizmo::Handle::ResizeBottom;
    }

    if (pointInHandle(screenMousePosition, ImVec2(screenRect.left, screenRect.centerY()))) {
        return RectGizmo::Handle::ResizeLeft;
    }

    if (pointInHandle(screenMousePosition, ImVec2(screenRect.right, screenRect.centerY()))) {
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
