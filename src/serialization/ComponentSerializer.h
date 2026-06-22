#pragma once

#include "math/Vector2F.h"

#include <string>

class Entity;

namespace tinyxml2 {
class XMLDocument;
class XMLElement;
}

struct ComponentSerializationContext {
    bool isRootEntity = false;
    Vector2F rootPosition = Vector2F::zero();
};

class IComponentSerializer {
public:
    virtual ~IComponentSerializer() = default;

    virtual const char* getTypeName() const = 0;
    virtual bool hasComponent(const Entity& entity) const = 0;
    virtual void save(tinyxml2::XMLDocument& document, tinyxml2::XMLElement& entityElement, const Entity& entity) const = 0;
    virtual bool load(
        const tinyxml2::XMLElement& componentElement,
        Entity& entity,
        const ComponentSerializationContext& context,
        std::string& errorMessage
    ) const = 0;

    virtual bool loadBeforeOtherComponents() const;
    virtual bool requiresTransformBeforeLoad() const;
};
