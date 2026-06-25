#pragma once

#include "serialization/ComponentSerializer.h"

#include <string>
#include <unordered_map>

class ScriptComponentSerializer : public IComponentSerializer {
public:
    using PrefabLocalGuidMap = std::unordered_map<std::string, std::string>;

    static void setPrefabLocalGuidMap(const PrefabLocalGuidMap* guidToPrefabIdMap);

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
