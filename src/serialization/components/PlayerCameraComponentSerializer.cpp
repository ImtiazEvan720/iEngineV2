#include "serialization/components/PlayerCameraComponentSerializer.h"

#include "Entity.h"
#include "components/PlayerCameraComponent.h"
#include "serialization/ComponentSerializationHelpers.h"

#include "tinyxml2.h"

const char* PlayerCameraComponentSerializer::getTypeName() const {
    return "PlayerCameraComponent";
}

bool PlayerCameraComponentSerializer::hasComponent(const Entity& entity) const {
    return entity.getComponent<PlayerCameraComponent>() != nullptr;
}

void PlayerCameraComponentSerializer::save(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const Entity& entity
) const {
    const PlayerCameraComponent* cameraComponent = entity.getComponent<PlayerCameraComponent>();
    if (cameraComponent == nullptr) {
        return;
    }

    tinyxml2::XMLElement* component =
        ComponentSerializationHelpers::addComponentElement(document, entityElement, getTypeName());
    ComponentSerializationHelpers::setComponentEnabledAttribute(*component, *cameraComponent);

    const Vector2F& offset = cameraComponent->getOffset();
    component->SetAttribute("zoom", cameraComponent->getZoom());
    component->SetAttribute("viewportWidth", cameraComponent->getViewportWidth());
    component->SetAttribute("viewportHeight", cameraComponent->getViewportHeight());
    component->SetAttribute("offsetX", offset.x);
    component->SetAttribute("offsetY", offset.y);
    component->SetAttribute("clampToBounds", ComponentSerializationHelpers::boolText(cameraComponent->shouldClampToBounds()));
    component->SetAttribute("minX", cameraComponent->getMinX());
    component->SetAttribute("minY", cameraComponent->getMinY());
    component->SetAttribute("maxX", cameraComponent->getMaxX());
    component->SetAttribute("maxY", cameraComponent->getMaxY());
}

bool PlayerCameraComponentSerializer::load(
    const tinyxml2::XMLElement& componentElement,
    Entity& entity,
    const ComponentSerializationContext& context,
    std::string& errorMessage
) const {
    (void)context;
    (void)errorMessage;

    PlayerCameraComponent& cameraComponent = entity.addComponent<PlayerCameraComponent>(
        componentElement.FloatAttribute("zoom", 1.0f),
        componentElement.FloatAttribute("viewportWidth", 1280.0f),
        componentElement.FloatAttribute("viewportHeight", 720.0f),
        Vector2F(
            componentElement.FloatAttribute("offsetX", 0.0f),
            componentElement.FloatAttribute("offsetY", 0.0f)
        ),
        ComponentSerializationHelpers::parseBool(componentElement.Attribute("clampToBounds"), false),
        componentElement.FloatAttribute("minX", 0.0f),
        componentElement.FloatAttribute("minY", 0.0f),
        componentElement.FloatAttribute("maxX", 1280.0f),
        componentElement.FloatAttribute("maxY", 720.0f)
    );
    ComponentSerializationHelpers::applyComponentEnabledAttribute(componentElement, cameraComponent);
    return true;
}

bool PlayerCameraComponentSerializer::requiresTransformBeforeLoad() const {
    return true;
}
