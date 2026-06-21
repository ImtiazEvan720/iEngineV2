#include "serialization/components/CollisionComponentSerializer.h"

#include "Entity.h"
#include "components/CollisionComponent.h"
#include "serialization/ComponentSerializationHelpers.h"

#include "tinyxml2.h"

#include <algorithm>
#include <cctype>

namespace {
const char* bodyTypeToString(CollisionComponent::BodyType bodyType) {
    switch (bodyType) {
        case CollisionComponent::BodyType::Kinematic:
            return "Kinematic";
        case CollisionComponent::BodyType::Dynamic:
            return "Dynamic";
        case CollisionComponent::BodyType::Static:
        default:
            return "Static";
    }
}

CollisionComponent::BodyType bodyTypeFromString(const char* value) {
    if (value == nullptr) {
        return CollisionComponent::BodyType::Static;
    }

    std::string text(value);
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::tolower(character));
                   });

    if (text == "kinematic") {
        return CollisionComponent::BodyType::Kinematic;
    }

    if (text == "dynamic") {
        return CollisionComponent::BodyType::Dynamic;
    }

    return CollisionComponent::BodyType::Static;
}
}

const char* CollisionComponentSerializer::getTypeName() const {
    return "CollisionComponent";
}

bool CollisionComponentSerializer::hasComponent(const Entity& entity) const {
    return entity.getComponent<CollisionComponent>() != nullptr;
}

void CollisionComponentSerializer::save(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const Entity& entity
) const {
    const CollisionComponent* collisionComponent = entity.getComponent<CollisionComponent>();
    if (collisionComponent == nullptr) {
        return;
    }

    tinyxml2::XMLElement* component =
        ComponentSerializationHelpers::addComponentElement(document, entityElement, getTypeName());
    const Vector2F& offset = collisionComponent->getOffset();
    component->SetAttribute("name", collisionComponent->getName().c_str());
    component->SetAttribute("width", collisionComponent->getWidth());
    component->SetAttribute("height", collisionComponent->getHeight());
    component->SetAttribute("offsetX", offset.x);
    component->SetAttribute("offsetY", offset.y);
    component->SetAttribute("bodyType", bodyTypeToString(collisionComponent->getBodyType()));
    component->SetAttribute("isSensor", ComponentSerializationHelpers::boolText(collisionComponent->isSensor()));
}

bool CollisionComponentSerializer::load(
    const tinyxml2::XMLElement& componentElement,
    Entity& entity,
    const ComponentSerializationContext& context,
    std::string& errorMessage
) const {
    (void)context;
    (void)errorMessage;

    const float width = componentElement.FloatAttribute("width", 1.0f);
    const float height = componentElement.FloatAttribute("height", 1.0f);
    const bool isSensor = ComponentSerializationHelpers::parseBool(componentElement.Attribute("isSensor"), false);
    const char* colliderName = componentElement.Attribute("name");
    CollisionComponent& collisionComponent = entity.addComponent<CollisionComponent>(
        width,
        height,
        bodyTypeFromString(componentElement.Attribute("bodyType")),
        isSensor,
        colliderName == nullptr ? "Collider" : colliderName
    );
    collisionComponent.setOffset(Vector2F(
        componentElement.FloatAttribute("offsetX", 0.0f),
        componentElement.FloatAttribute("offsetY", 0.0f)
    ));

    return true;
}

bool CollisionComponentSerializer::requiresTransformBeforeLoad() const {
    return true;
}
