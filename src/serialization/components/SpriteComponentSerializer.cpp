#include "serialization/components/SpriteComponentSerializer.h"

#include "Entity.h"
#include "components/SpriteComponent.h"
#include "misc/TextureAsset.h"
#include "serialization/ComponentSerializationHelpers.h"

#include "tinyxml2.h"

#include <sstream>

const char* SpriteComponentSerializer::getTypeName() const {
    return "SpriteComponent";
}

bool SpriteComponentSerializer::hasComponent(const Entity& entity) const {
    return entity.getComponent<SpriteComponent>() != nullptr;
}

void SpriteComponentSerializer::save(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const Entity& entity
) const {
    const SpriteComponent* spriteComponent = entity.getComponent<SpriteComponent>();
    if (spriteComponent == nullptr) {
        return;
    }

    tinyxml2::XMLElement* component =
        ComponentSerializationHelpers::addComponentElement(document, entityElement, getTypeName());
    ComponentSerializationHelpers::setSpriteAttributes(*component, spriteComponent->getSprite());
}

bool SpriteComponentSerializer::load(
    const tinyxml2::XMLElement& componentElement,
    Entity& entity,
    const ComponentSerializationContext& context,
    std::string& errorMessage
) const {
    (void)context;

    TextureAsset* textureAsset = ComponentSerializationHelpers::getTextureAsset(componentElement);
    if (textureAsset == nullptr || textureAsset->getTextureHandle() == nullptr) {
        std::ostringstream stream;
        stream << "Prefab sprite texture is missing: "
               << (componentElement.Attribute("texture") == nullptr ? "<none>" : componentElement.Attribute("texture"));
        errorMessage = stream.str();
        return false;
    }

    entity.addComponent<SpriteComponent>(
        ComponentSerializationHelpers::makeSpriteFromAttributes(componentElement, *textureAsset)
    );
    return true;
}
