#pragma once

#include "serialization/ComponentSerializer.h"

class PlayerControllerComponentSerializer : public IComponentSerializer {
public:
    const char* getTypeName() const override;
    bool hasComponent(const Entity& entity) const override;
    void save(tinyxml2::XMLDocument& document, tinyxml2::XMLElement& entityElement, const Entity& entity) const override;
    bool load(
        const tinyxml2::XMLElement& componentElement,
        Entity& entity,
        const ComponentSerializationContext& context,
        std::string& errorMessage
    ) const override;
};

class BrickComponentSerializer : public IComponentSerializer {
public:
    const char* getTypeName() const override;
    bool hasComponent(const Entity& entity) const override;
    void save(tinyxml2::XMLDocument& document, tinyxml2::XMLElement& entityElement, const Entity& entity) const override;
    bool load(
        const tinyxml2::XMLElement& componentElement,
        Entity& entity,
        const ComponentSerializationContext& context,
        std::string& errorMessage
    ) const override;
};
