#include "serialization/components/GameComponentSerializers.h"

#include "Entity.h"
#include "components/PlayerController.h"
#include "components/ScriptComponent.h"
#include "game/Brick.h"
#include "serialization/ComponentSerializationHelpers.h"

#include "tinyxml2.h"

namespace {
void addMarkerComponent(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const char* type
) {
    ComponentSerializationHelpers::addComponentElement(document, entityElement, type);
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
    if (hasComponent(entity)) {
        addMarkerComponent(document, entityElement, getTypeName());
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

    entity.addComponent<ScriptComponent>("Assets/Scripts/player_controller.lua");
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
    if (hasComponent(entity)) {
        addMarkerComponent(document, entityElement, getTypeName());
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

    entity.addComponent<Brick>();
    return true;
}
