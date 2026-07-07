#pragma once

#include "serialization/ComponentSerializer.h"

#include <memory>
#include <string>
#include <vector>

namespace tinyxml2 {
class XMLDocument;
class XMLElement;
}

class ComponentSerializerRegistry {
public:
    static ComponentSerializerRegistry& getInstance();

    void saveComponents(tinyxml2::XMLDocument& document, tinyxml2::XMLElement& entityElement, const Entity& entity) const;
    bool loadComponents(
        const tinyxml2::XMLElement& entityElement,
        Entity& entity,
        const ComponentSerializationContext& context,
        std::string& errorMessage
    ) const;

private:
    ComponentSerializerRegistry();

    void registerSerializer(std::unique_ptr<IComponentSerializer> serializer);
    const IComponentSerializer* findSerializer(const std::string& typeName) const;
    bool hasSerializedComponent(const tinyxml2::XMLElement& entityElement, const char* typeName) const;
    bool serializedComponentsRequireTransform(const tinyxml2::XMLElement& entityElement) const;
    bool serializedComponentsRequireRectTransform(const tinyxml2::XMLElement& entityElement) const;

    std::vector<std::unique_ptr<IComponentSerializer>> serializers;
};
