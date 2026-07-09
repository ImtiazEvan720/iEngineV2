#include "editor/components/CollisionComponentDrawer.h"

#include "Entity.h"
#include "components/CollisionComponent.h"
#include "math/Vector2F.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <algorithm>

namespace {
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

CollisionComponent::BodyType bodyTypeFromIndex(int index) {
    switch (index) {
        case 1:
            return CollisionComponent::BodyType::Kinematic;
        case 2:
            return CollisionComponent::BodyType::Dynamic;
        case 0:
        default:
            return CollisionComponent::BodyType::Static;
    }
}
}

void CollisionComponentDrawer::draw(
    Entity& entity,
    Component& component,
    std::string& statusMessage
) {
    auto* collisionComponent = dynamic_cast<CollisionComponent*>(&component);
    if (collisionComponent == nullptr) {
        ImGui::TextDisabled("Invalid CollisionComponent.");
        return;
    }

    if (editState.entityId != entity.getId()) {
        editState.entityId = entity.getId();
        const Vector2F& offset = collisionComponent->getOffset();
        editState.offset[0] = offset.x;
        editState.offset[1] = offset.y;
        editState.size[0] = collisionComponent->getWidth();
        editState.size[1] = collisionComponent->getHeight();
        editState.rotation = collisionComponent->getRotation();
        editState.bodyType = bodyTypeToIndex(collisionComponent->getBodyType());
        editState.sensor = collisionComponent->isSensor();
        editState.fixedRotation = collisionComponent->isFixedRotation();
        editState.name = collisionComponent->getName();
    }

    ImGui::InputText("Name", &editState.name);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        collisionComponent->setName(editState.name);
        statusMessage = "Updated CollisionComponent name.";
    }

    ImGui::InputFloat2("Offset", editState.offset, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        collisionComponent->setOffset(Vector2F(editState.offset[0], editState.offset[1]));
        statusMessage = "Updated CollisionComponent offset.";
    }

    ImGui::InputFloat2("Size", editState.size, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        editState.size[0] = std::max(0.001f, editState.size[0]);
        editState.size[1] = std::max(0.001f, editState.size[1]);
        collisionComponent->setSize(editState.size[0], editState.size[1]);
        statusMessage = "Updated CollisionComponent size.";
    }

    if (ImGui::SliderFloat("Rotation", &editState.rotation, -180.0f, 180.0f, "%.2f deg")) {
        collisionComponent->setRotation(editState.rotation);
        statusMessage = "Updated CollisionComponent rotation.";
    }

    if (ImGui::Combo("Body Type", &editState.bodyType, "Static\0Kinematic\0Dynamic\0")) {
        collisionComponent->setBodyType(bodyTypeFromIndex(editState.bodyType));
        statusMessage = "Updated CollisionComponent body type.";
    }

    if (ImGui::Checkbox("Sensor", &editState.sensor)) {
        collisionComponent->setSensor(editState.sensor);
        statusMessage = "Updated CollisionComponent sensor.";
    }

    if (ImGui::Checkbox("Fixed Rotation", &editState.fixedRotation)) {
        collisionComponent->setFixedRotation(editState.fixedRotation);
        statusMessage = "Updated CollisionComponent fixed rotation.";
    }
}
