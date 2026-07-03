#include "editor/components/PlayerCameraComponentDrawer.h"

#include "Entity.h"
#include "components/PlayerCameraComponent.h"
#include "math/Vector2F.h"

#include "imgui.h"

#include <algorithm>

void PlayerCameraComponentDrawer::draw(
    Entity& entity,
    Component& component,
    std::string& statusMessage
) {
    auto* cameraComponent = dynamic_cast<PlayerCameraComponent*>(&component);
    if (cameraComponent == nullptr) {
        ImGui::TextDisabled("Invalid PlayerCameraComponent.");
        return;
    }

    if (editState.entityId != entity.getId()) {
        editState.entityId = entity.getId();
        const Vector2F& offset = cameraComponent->getOffset();
        editState.zoom = cameraComponent->getZoom();
        editState.viewportSize[0] = cameraComponent->getViewportWidth();
        editState.viewportSize[1] = cameraComponent->getViewportHeight();
        editState.offset[0] = offset.x;
        editState.offset[1] = offset.y;
        editState.clampToBounds = cameraComponent->shouldClampToBounds();
        editState.boundsMin[0] = cameraComponent->getMinX();
        editState.boundsMin[1] = cameraComponent->getMinY();
        editState.boundsMax[0] = cameraComponent->getMaxX();
        editState.boundsMax[1] = cameraComponent->getMaxY();
    }

    ImGui::InputFloat("Zoom", &editState.zoom, 0.0f, 0.0f, "%.3f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        editState.zoom = std::max(0.001f, editState.zoom);
        cameraComponent->setZoom(editState.zoom);
        statusMessage = "Updated PlayerCameraComponent zoom.";
    }

    ImGui::InputFloat2("Viewport Size", editState.viewportSize, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        editState.viewportSize[0] = std::max(1.0f, editState.viewportSize[0]);
        editState.viewportSize[1] = std::max(1.0f, editState.viewportSize[1]);
        cameraComponent->setViewportWidth(editState.viewportSize[0]);
        cameraComponent->setViewportHeight(editState.viewportSize[1]);
        statusMessage = "Updated PlayerCameraComponent viewport size.";
    }

    ImGui::InputFloat2("Offset", editState.offset, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        cameraComponent->setOffset(Vector2F(editState.offset[0], editState.offset[1]));
        statusMessage = "Updated PlayerCameraComponent offset.";
    }

    if (ImGui::Checkbox("Clamp To Bounds", &editState.clampToBounds)) {
        cameraComponent->setClampToBounds(editState.clampToBounds);
        statusMessage = "Updated PlayerCameraComponent bounds clamp.";
    }

    ImGui::InputFloat2("Bounds Min", editState.boundsMin, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        cameraComponent->setBounds(
            editState.boundsMin[0],
            editState.boundsMin[1],
            editState.boundsMax[0],
            editState.boundsMax[1]
        );
        statusMessage = "Updated PlayerCameraComponent bounds minimum.";
    }

    ImGui::InputFloat2("Bounds Max", editState.boundsMax, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        cameraComponent->setBounds(
            editState.boundsMin[0],
            editState.boundsMin[1],
            editState.boundsMax[0],
            editState.boundsMax[1]
        );
        statusMessage = "Updated PlayerCameraComponent bounds maximum.";
    }
}
