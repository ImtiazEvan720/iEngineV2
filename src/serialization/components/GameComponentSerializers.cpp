#include "serialization/components/GameComponentSerializers.h"

#include "Entity.h"
#include "components/PlayerController.h"
#include "components/ScriptComponent.h"
#include "game/Brick.h"
#include "serialization/ComponentSerializationHelpers.h"

#include "tinyxml2.h"

namespace {
tinyxml2::XMLElement* addMarkerComponent(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const char* type
) {
    return ComponentSerializationHelpers::addComponentElement(document, entityElement, type);
}
}

const char* PlayerControllerComponentSerializer::getTypeName() const {
    return "PlayerController";
}

bool PlayerControllerComponentSerializer::hasComponent(const Entity& entity) const {
    return entity.getComponent<PlayerController>() != nullptr;
}

void PlayerControllerComponentSerializer::save(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const Entity& entity
) const {
    const PlayerController* playerController = entity.getComponent<PlayerController>();
    if (playerController != nullptr) {
        tinyxml2::XMLElement* component = addMarkerComponent(document, entityElement, getTypeName());
        ComponentSerializationHelpers::setComponentEnabledAttribute(*component, *playerController);
    }
}

bool PlayerControllerComponentSerializer::load(
    const tinyxml2::XMLElement& componentElement,
    Entity& entity,
    const ComponentSerializationContext& context,
    std::string& errorMessage
) const {
    (void)componentElement;
    (void)context;
    (void)errorMessage;

    ScriptComponent& scriptComponent = entity.addComponent<ScriptComponent>("Assets/Scripts/player_controller.lua");
    ComponentSerializationHelpers::applyComponentEnabledAttribute(componentElement, scriptComponent);
    return true;
}

const char* BrickComponentSerializer::getTypeName() const {
    return "Brick";
}

bool BrickComponentSerializer::hasComponent(const Entity& entity) const {
    return entity.getComponent<Brick>() != nullptr;
}

void BrickComponentSerializer::save(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const Entity& entity
) const {
    const Brick* brick = entity.getComponent<Brick>();
    if (brick != nullptr) {
        tinyxml2::XMLElement* component = addMarkerComponent(document, entityElement, getTypeName());
        ComponentSerializationHelpers::setComponentEnabledAttribute(*component, *brick);
    }
}

bool BrickComponentSerializer::load(
    const tinyxml2::XMLElement& componentElement,
    Entity& entity,
    const ComponentSerializationContext& context,
    std::string& errorMessage
) const {
    (void)componentElement;
    (void)context;
    (void)errorMessage;

    Brick& brick = entity.addComponent<Brick>();
    ComponentSerializationHelpers::applyComponentEnabledAttribute(componentElement, brick);
    return true;
}
