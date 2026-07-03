#include "editor/components/RectTransformComponentDrawer.h"

#include "Entity.h"
#include "components/RectTransformComponent.h"
#include "math/Vector2F.h"

#include "imgui.h"

#include <algorithm>

void RectTransformComponentDrawer::draw(
    Entity& entity,
    Component& component,
    std::string& statusMessage
) {
    auto* rectTransform = dynamic_cast<RectTransformComponent*>(&component);
    if (rectTransform == nullptr) {
        ImGui::TextDisabled("Invalid RectTransformComponent.");
        return;
    }

    if (editState.entityId != entity.getId()) {
        editState.entityId = entity.getId();
        const Vector2F& anchoredPosition = rectTransform->getAnchoredPosition();
        const Vector2F& size = rectTransform->getSize();
        const Vector2F& pivot = rectTransform->getPivot();
        editState.anchoredPosition[0] = anchoredPosition.x;
        editState.anchoredPosition[1] = anchoredPosition.y;
        editState.size[0] = size.x;
        editState.size[1] = size.y;
        editState.pivot[0] = pivot.x;
        editState.pivot[1] = pivot.y;
        editState.rotation = rectTransform->getRotation();
    }

    ImGui::InputFloat2("Anchored Position", editState.anchoredPosition, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        rectTransform->setAnchoredPosition(Vector2F(
            editState.anchoredPosition[0],
            editState.anchoredPosition[1]
        ));
        statusMessage = "Updated RectTransformComponent anchored position.";
    }

    ImGui::InputFloat2("Size", editState.size, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        editState.size[0] = std::max(1.0f, editState.size[0]);
        editState.size[1] = std::max(1.0f, editState.size[1]);
        rectTransform->setSize(Vector2F(editState.size[0], editState.size[1]));
        statusMessage = "Updated RectTransformComponent size.";
    }

    ImGui::InputFloat2("Pivot", editState.pivot, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        editState.pivot[0] = std::clamp(editState.pivot[0], 0.0f, 1.0f);
        editState.pivot[1] = std::clamp(editState.pivot[1], 0.0f, 1.0f);
        rectTransform->setPivot(Vector2F(editState.pivot[0], editState.pivot[1]));
        statusMessage = "Updated RectTransformComponent pivot.";
    }

    ImGui::InputFloat("Rotation", &editState.rotation, 0.0f, 0.0f, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        rectTransform->setRotation(editState.rotation);
        statusMessage = "Updated RectTransformComponent rotation.";
    }
}
