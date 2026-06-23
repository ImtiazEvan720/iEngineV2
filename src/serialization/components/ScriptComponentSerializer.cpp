#include "serialization/components/ScriptComponentSerializer.h"

#include "Entity.h"
#include "components/ScriptComponent.h"
#include "serialization/ComponentSerializationHelpers.h"

#include "tinyxml2.h"

#include <utility>
#include <vector>

namespace {
std::vector<ScriptProperty> loadScriptProperties(const tinyxml2::XMLElement& componentElement) {
    std::vector<ScriptProperty> properties;

    for (const tinyxml2::XMLElement* propertyElement = componentElement.FirstChildElement("property");
         propertyElement != nullptr;
         propertyElement = propertyElement->NextSiblingElement("property")) {
        const char* name = propertyElement->Attribute("name");
        if (name == nullptr || name[0] == '\0') {
            continue;
        }

        const char* type = propertyElement->Attribute("type");
        const char* value = propertyElement->Attribute("value");

        ScriptProperty property;
        property.name = name;
        property.type = scriptPropertyTypeFromString(type == nullptr ? "string" : type);
        scriptPropertySetValueFromString(property, value == nullptr ? "" : value);
        properties.push_back(std::move(property));
    }

    return properties;
}

void saveScriptProperties(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& componentElement,
    const ScriptComponent& scriptComponent
) {
    for (const ScriptProperty& property : scriptComponent.getProperties()) {
        if (property.name.empty()) {
            continue;
        }

        tinyxml2::XMLElement* propertyElement = document.NewElement("property");
        propertyElement->SetAttribute("name", property.name.c_str());
        propertyElement->SetAttribute("type", scriptPropertyTypeToString(property.type));
        propertyElement->SetAttribute("value", scriptPropertyValueToString(property).c_str());
        componentElement.InsertEndChild(propertyElement);
    }
}
}

const char* ScriptComponentSerializer::getTypeName() const {
    return "ScriptComponent";
}

bool ScriptComponentSerializer::hasComponent(const Entity& entity) const {
    return entity.getComponent<ScriptComponent>() != nullptr;
}

void ScriptComponentSerializer::save(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const Entity& entity
) const {
    const ScriptComponent* scriptComponent = entity.getComponent<ScriptComponent>();
    if (scriptComponent == nullptr) {
        return;
    }

    tinyxml2::XMLElement* component =
        ComponentSerializationHelpers::addComponentElement(document, entityElement, getTypeName());
    component->SetAttribute("path", scriptComponent->getScriptPath().c_str());
    saveScriptProperties(document, *component, *scriptComponent);
}

bool ScriptComponentSerializer::load(
    const tinyxml2::XMLElement& componentElement,
    Entity& entity,
    const ComponentSerializationContext& context,
    std::string& errorMessage
) const {
    (void)context;
    (void)errorMessage;

    const char* scriptPath = componentElement.Attribute("path");
    if (scriptPath != nullptr && scriptPath[0] != '\0') {
        entity.addComponent<ScriptComponent>(scriptPath, loadScriptProperties(componentElement));
    }

    return true;
}
