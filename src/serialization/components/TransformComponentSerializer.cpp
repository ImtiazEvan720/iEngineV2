#include "serialization/components/TransformComponentSerializer.h"

#include "Entity.h"
#include "components/TransformComponent.h"
#include "serialization/ComponentSerializationHelpers.h"

#include "tinyxml2.h"

const char* TransformComponentSerializer::getTypeName() const {
    return "TransformComponent";
}

bool TransformComponentSerializer::hasComponent(const Entity& entity) const {
    return entity.getComponent<TransformComponent>() != nullptr;
}

void TransformComponentSerializer::save(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const Entity& entity
) const {
    const TransformComponent* transform = entity.getComponent<TransformComponent>();
    if (transform == nullptr) {
        return;
    }

    tinyxml2::XMLElement* component =
        ComponentSerializationHelpers::addComponentElement(document, entityElement, getTypeName());
    const Vector2F& position = transform->getPosition();
    component->SetAttribute("x", position.x);
    component->SetAttribute("y", position.y);
    component->SetAttribute("rotation", transform->getRotation());
}

bool TransformComponentSerializer::load(
    const tinyxml2::XMLElement& componentElement,
    Entity& entity,
    const ComponentSerializationContext& context,
    std::string& errorMessage
) const {
    (void)errorMessage;

    Vector2F position = context.rootPosition;
    if (!context.isRootEntity) {
        position = Vector2F(
            componentElement.FloatAttribute("x", 0.0f),
            componentElement.FloatAttribute("y", 0.0f)
        );
    }

    const float rotation = componentElement.FloatAttribute("rotation", 0.0f);

    TransformComponent* transform = entity.getComponent<TransformComponent>();
    if (transform == nullptr) {
        entity.addComponent<TransformComponent>(position, rotation);
    } else {
        transform->setPosition(position);
        transform->setRotation(rotation);
    }

    return true;
}

bool TransformComponentSerializer::loadBeforeOtherComponents() const {
    return true;
}
