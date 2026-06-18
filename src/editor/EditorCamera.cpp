#include "editor/EditorCamera.h"

#include "math/Vector2F.h"
#include "misc/Camera2D.h"
#include "system/Renderer.h"

#include "imgui.h"

#include <cmath>

void EditorCamera::beginFrame() {
    consumedPinchZoomThisFrame = false;
}

void EditorCamera::handleZoom(bool enabled, std::string& statusMessage) {
    if (!enabled) {
        return;
    }

    const ImGuiPayload* activePayload = ImGui::GetDragDropPayload();
    if (activePayload != nullptr) {
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    Renderer& renderer = Renderer::getInstance();
    const float pinchZoomFactor = renderer.consumePendingPinchZoomFactor();
    const bool pinchZoom =
        std::isfinite(pinchZoomFactor)
        && pinchZoomFactor > 0.0f
        && pinchZoomFactor != 1.0f;
    const bool zoomGesture = io.KeyCtrl || io.KeySuper;
    const bool wheelZoom = zoomGesture && io.MouseWheel != 0.0f;

    if (io.WantCaptureMouse || (!pinchZoom && !wheelZoom)) {
        return;
    }

    float zoomMultiplier = 1.0f;
    if (pinchZoom) {
        zoomMultiplier *= pinchZoomFactor;
        consumedPinchZoomThisFrame = true;
    }

    if (wheelZoom) {
        zoomMultiplier *= std::pow(1.1f, io.MouseWheel);
    }

    renderer.getCamera().zoomAtScreenPoint(
        zoomMultiplier,
        Vector2F(io.MousePos.x, io.MousePos.y),
        getViewport()
    );

    statusMessage = "Viewport zoom: " + std::to_string(renderer.getCamera().getZoom());
}

void EditorCamera::handlePan(bool enabled) {
    constexpr float WheelPanPixels = 48.0f;

    if (!enabled) {
        return;
    }

    const ImGuiPayload* activePayload = ImGui::GetDragDropPayload();
    if (activePayload != nullptr) {
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    const bool zoomGesture = io.KeyCtrl || io.KeySuper;
    const bool wheelPan =
        !zoomGesture
        && !consumedPinchZoomThisFrame
        && (io.MouseWheel != 0.0f || io.MouseWheelH != 0.0f);
    const bool dragPan = isPanActive();

    if (io.WantCaptureMouse || (!dragPan && !wheelPan)) {
        return;
    }

    Camera2D& camera = Renderer::getInstance().getCamera();
    const float zoom = camera.getZoom();
    if (zoom <= 0.0f) {
        return;
    }

    Vector2F position = camera.getPosition();

    if (dragPan) {
        position.x -= io.MouseDelta.x / zoom;
        position.y -= io.MouseDelta.y / zoom;
    }

    if (wheelPan) {
        position.x -= (io.MouseWheelH * WheelPanPixels) / zoom;
        position.y -= (io.MouseWheel * WheelPanPixels) / zoom;
    }

    camera.setPosition(position);
}

bool EditorCamera::isPanActive() const {
    const bool middleDrag = ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f);
    const bool spaceLeftDrag =
        ImGui::IsKeyDown(ImGuiKey_Space)
        && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f);

    return middleDrag || spaceLeftDrag;
}

RenderRect EditorCamera::getViewport() const {
    RenderRect viewport = Renderer::getInstance().getViewport();
    if (viewport.width > 0.0f && viewport.height > 0.0f) {
        return viewport;
    }

    const ImGuiIO& io = ImGui::GetIO();
    return RenderRect{
        0.0f,
        0.0f,
        io.DisplaySize.x,
        io.DisplaySize.y
    };
}
