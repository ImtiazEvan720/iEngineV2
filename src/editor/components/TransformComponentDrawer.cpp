#include "editor/components/TransformComponentDrawer.h"

#include "Entity.h"
#include "components/TransformComponent.h"
#include "math/Vector2F.h"

#include "imgui.h"

void TransformComponentDrawer::draw(
    Entity& entity,
    Component& component,
    std::string& statusMessage
) {
    auto* transform = dynamic_cast<TransformComponent*>(&component);
    if (transform == nullptr) {
        ImGui::TextDisabled("Invalid TransformComponent.");
        return;
    }

    if (editState.entityId != entity.getId()) {
        editState.entityId = entity.getId();
        const Vector2F& position = transform->getPosition();
        editState.position[0] = position.x;
        editState.position[1] = position.y;
        editState.rotation = transform->getRotation();
    }

    ImGui::InputFloat2("Position", editState.position, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        transform->setPosition(Vector2F(editState.position[0], editState.position[1]));
        statusMessage = "Updated TransformComponent position.";
    }

    ImGui::InputFloat("Rotation", &editState.rotation, 0.0f, 0.0f, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        transform->setRotation(editState.rotation);
        statusMessage = "Updated TransformComponent rotation.";
    }

    const Vector2F worldPosition = transform->getWorldPosition();
    ImGui::Text("World Position: %.2f, %.2f", worldPosition.x, worldPosition.y);
    ImGui::Text("World Rotation: %.2f", transform->getWorldRotation());
}
