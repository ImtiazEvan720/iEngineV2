#include "serialization/components/ScriptComponentSerializer.h"

#include "Entity.h"
#include "components/ScriptComponent.h"
#include "serialization/ComponentSerializationHelpers.h"

#include "tinyxml2.h"

#include <string>
#include <utility>
#include <vector>

namespace {
const ScriptComponentSerializer::PrefabLocalGuidMap* prefabLocalGuidMap = nullptr;

ScriptEntityReferenceScope loadEntityReferenceScope(const tinyxml2::XMLElement& element) {
    return scriptEntityReferenceScopeFromString(
        element.Attribute("scope") == nullptr ? "Level" : element.Attribute("scope")
    );
}

void saveEntityReferenceScope(tinyxml2::XMLElement& element, ScriptEntityReferenceScope scope) {
    if (scope != ScriptEntityReferenceScope::Level) {
        element.SetAttribute("scope", scriptEntityReferenceScopeToString(scope));
    }
}

bool tryMakePrefabLocalEntityReference(std::string& value, ScriptEntityReferenceScope& scope) {
    if (prefabLocalGuidMap == nullptr || value.empty()) {
        return false;
    }

    const auto iterator = prefabLocalGuidMap->find(value);
    if (iterator == prefabLocalGuidMap->end()) {
        return false;
    }

    value = iterator->second;
    scope = ScriptEntityReferenceScope::PrefabLocal;
    return true;
}

ScriptValue loadScriptValue(const tinyxml2::XMLElement& valueElement, ScriptValueType type) {
    ScriptValue value;
    value.type = type;
    if (type == ScriptValueType::Entity) {
        value.entityReferenceScope = loadEntityReferenceScope(valueElement);
    }
    scriptValueSetValueFromString(
        value,
        valueElement.Attribute("value") == nullptr ? "" : valueElement.Attribute("value")
    );
    return value;
}

ScriptProperty loadScriptProperty(const tinyxml2::XMLElement& propertyElement) {
    ScriptProperty property;
    property.name = propertyElement.Attribute("name") == nullptr ? "" : propertyElement.Attribute("name");
    property.type = scriptPropertyTypeFromString(
        propertyElement.Attribute("type") == nullptr ? "string" : propertyElement.Attribute("type")
    );
    if (property.type == ScriptPropertyType::Entity) {
        property.entityReferenceScope = loadEntityReferenceScope(propertyElement);
    }

    if (property.type == ScriptPropertyType::Array) {
        property.elementType = scriptValueTypeFromString(
            propertyElement.Attribute("elementType") == nullptr ? "string" : propertyElement.Attribute("elementType")
        );

        for (const tinyxml2::XMLElement* itemElement = propertyElement.FirstChildElement("item");
             itemElement != nullptr;
             itemElement = itemElement->NextSiblingElement("item")) {
            property.arrayValue.push_back(loadScriptValue(*itemElement, property.elementType));
        }

        return property;
    }

    if (property.type == ScriptPropertyType::Map) {
        property.mapValueType = scriptValueTypeFromString(
            propertyElement.Attribute("valueType") == nullptr ? "string" : propertyElement.Attribute("valueType")
        );

        for (const tinyxml2::XMLElement* entryElement = propertyElement.FirstChildElement("entry");
             entryElement != nullptr;
             entryElement = entryElement->NextSiblingElement("entry")) {
            ScriptMapEntry entry;
            entry.key = entryElement->Attribute("key") == nullptr ? "" : entryElement->Attribute("key");
            entry.value = loadScriptValue(*entryElement, property.mapValueType);
            property.mapValue.push_back(std::move(entry));
        }

        return property;
    }

    scriptPropertySetValueFromString(
        property,
        propertyElement.Attribute("value") == nullptr ? "" : propertyElement.Attribute("value")
    );
    return property;
}

std::vector<ScriptProperty> loadScriptProperties(const tinyxml2::XMLElement& componentElement) {
    std::vector<ScriptProperty> properties;

    for (const tinyxml2::XMLElement* propertyElement = componentElement.FirstChildElement("property");
         propertyElement != nullptr;
         propertyElement = propertyElement->NextSiblingElement("property")) {
        const char* name = propertyElement->Attribute("name");
        if (name == nullptr || name[0] == '\0') {
            continue;
        }

        properties.push_back(loadScriptProperty(*propertyElement));
    }

    return properties;
}

void saveScriptValue(tinyxml2::XMLElement& valueElement, const ScriptValue& value) {
    std::string serializedValue = scriptValueToString(value);
    ScriptEntityReferenceScope serializedScope = value.entityReferenceScope;
    if (value.type == ScriptValueType::Entity) {
        tryMakePrefabLocalEntityReference(serializedValue, serializedScope);
        saveEntityReferenceScope(valueElement, serializedScope);
    }

    valueElement.SetAttribute("value", serializedValue.c_str());
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

        if (property.type == ScriptPropertyType::Array) {
            propertyElement->SetAttribute("elementType", scriptValueTypeToString(property.elementType));
            for (const ScriptValue& value : property.arrayValue) {
                tinyxml2::XMLElement* itemElement = document.NewElement("item");
                saveScriptValue(*itemElement, value);
                propertyElement->InsertEndChild(itemElement);
            }
        } else if (property.type == ScriptPropertyType::Map) {
            propertyElement->SetAttribute("valueType", scriptValueTypeToString(property.mapValueType));
            for (const ScriptMapEntry& entry : property.mapValue) {
                tinyxml2::XMLElement* entryElement = document.NewElement("entry");
                entryElement->SetAttribute("key", entry.key.c_str());
                saveScriptValue(*entryElement, entry.value);
                propertyElement->InsertEndChild(entryElement);
            }
        } else {
            std::string serializedValue = scriptPropertyValueToString(property);
            ScriptEntityReferenceScope serializedScope = property.entityReferenceScope;
            if (property.type == ScriptPropertyType::Entity) {
                tryMakePrefabLocalEntityReference(serializedValue, serializedScope);
                saveEntityReferenceScope(*propertyElement, serializedScope);
            }

            propertyElement->SetAttribute("value", serializedValue.c_str());
        }

        componentElement.InsertEndChild(propertyElement);
    }
}
}

void ScriptComponentSerializer::setPrefabLocalGuidMap(const PrefabLocalGuidMap* guidToPrefabIdMap) {
    prefabLocalGuidMap = guidToPrefabIdMap;
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
