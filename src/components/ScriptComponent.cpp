#include "components/ScriptComponent.h"

#include "misc/Level.h"
#include "system/ScriptSystem.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {
std::string makeUniquePropertyName(
    const std::vector<ScriptProperty>& properties,
    const std::string& requestedName
) {
    const std::string baseName = requestedName.empty() ? "property" : requestedName;
    std::string candidate = baseName;
    int suffix = 1;

    auto propertyExists = [&properties](const std::string& name) {
        for (const ScriptProperty& property : properties) {
            if (property.name == name) {
                return true;
            }
        }

        return false;
    };

    while (propertyExists(candidate)) {
        candidate = baseName + std::to_string(suffix);
        ++suffix;
    }

    return candidate;
}

std::string trimString(const std::string& value) {
    const auto begin = std::find_if_not(
        value.begin(),
        value.end(),
        [](unsigned char character) {
            return std::isspace(character) != 0;
        }
    );

    const auto end = std::find_if_not(
        value.rbegin(),
        value.rend(),
        [](unsigned char character) {
            return std::isspace(character) != 0;
        }
    ).base();

    if (begin >= end) {
        return "";
    }

    return std::string(begin, end);
}

Vector2F parseVector2Value(const std::string& value) {
    const std::string trimmed = trimString(value);
    const std::size_t commaPosition = trimmed.find(',');
    if (commaPosition == std::string::npos) {
        return Vector2F::zero();
    }

    try {
        const float x = std::stof(trimmed.substr(0, commaPosition));
        const float y = std::stof(trimmed.substr(commaPosition + 1));
        return Vector2F(x, y);
    } catch (const std::invalid_argument&) {
        return Vector2F::zero();
    } catch (const std::out_of_range&) {
        return Vector2F::zero();
    }
}

std::string vector2ToString(const Vector2F& value) {
    std::ostringstream stream;
    stream << value.x << "," << value.y;
    return stream.str();
}
}

const char* scriptPropertyTypeToString(ScriptPropertyType type) {
    switch (type) {
        case ScriptPropertyType::Int:
            return "int";
        case ScriptPropertyType::Float:
            return "float";
        case ScriptPropertyType::Bool:
            return "bool";
        case ScriptPropertyType::Prefab:
            return "prefab";
        case ScriptPropertyType::Vector2:
            return "vector2";
        case ScriptPropertyType::Entity:
            return "entity";
        case ScriptPropertyType::Array:
            return "array";
        case ScriptPropertyType::Map:
            return "map";
        case ScriptPropertyType::String:
        default:
            return "string";
    }
}

ScriptPropertyType scriptPropertyTypeFromString(const std::string& value) {
    if (value == "int" || value == "Int") {
        return ScriptPropertyType::Int;
    }

    if (value == "float" || value == "Float") {
        return ScriptPropertyType::Float;
    }

    if (value == "bool" || value == "Bool") {
        return ScriptPropertyType::Bool;
    }

    if (value == "prefab" || value == "Prefab") {
        return ScriptPropertyType::Prefab;
    }

    if (value == "vector2" || value == "Vector2") {
        return ScriptPropertyType::Vector2;
    }

    if (value == "entity" || value == "Entity") {
        return ScriptPropertyType::Entity;
    }

    if (value == "array" || value == "Array") {
        return ScriptPropertyType::Array;
    }

    if (value == "map" || value == "Map" || value == "hashmap" || value == "HashMap") {
        return ScriptPropertyType::Map;
    }

    return ScriptPropertyType::String;
}

const char* scriptValueTypeToString(ScriptValueType type) {
    switch (type) {
        case ScriptValueType::Int:
            return "int";
        case ScriptValueType::Float:
            return "float";
        case ScriptValueType::Bool:
            return "bool";
        case ScriptValueType::Prefab:
            return "prefab";
        case ScriptValueType::Vector2:
            return "vector2";
        case ScriptValueType::Entity:
            return "entity";
        case ScriptValueType::String:
        default:
            return "string";
    }
}

ScriptValueType scriptValueTypeFromString(const std::string& value) {
    if (value == "int" || value == "Int") {
        return ScriptValueType::Int;
    }

    if (value == "float" || value == "Float") {
        return ScriptValueType::Float;
    }

    if (value == "bool" || value == "Bool") {
        return ScriptValueType::Bool;
    }

    if (value == "prefab" || value == "Prefab") {
        return ScriptValueType::Prefab;
    }

    if (value == "vector2" || value == "Vector2") {
        return ScriptValueType::Vector2;
    }

    if (value == "entity" || value == "Entity") {
        return ScriptValueType::Entity;
    }

    return ScriptValueType::String;
}

const char* scriptEntityReferenceScopeToString(ScriptEntityReferenceScope scope) {
    switch (scope) {
        case ScriptEntityReferenceScope::PrefabLocal:
            return "PrefabLocal";
        case ScriptEntityReferenceScope::Level:
        default:
            return "Level";
    }
}

ScriptEntityReferenceScope scriptEntityReferenceScopeFromString(const std::string& value) {
    if (value == "PrefabLocal" || value == "prefabLocal" || value == "prefab" || value == "Prefab") {
        return ScriptEntityReferenceScope::PrefabLocal;
    }

    return ScriptEntityReferenceScope::Level;
}

std::string scriptValueToString(const ScriptValue& value) {
    switch (value.type) {
        case ScriptValueType::Int:
            return std::to_string(value.intValue);
        case ScriptValueType::Float: {
            std::ostringstream stream;
            stream << value.floatValue;
            return stream.str();
        }
        case ScriptValueType::Bool:
            return value.boolValue ? "true" : "false";
        case ScriptValueType::Vector2:
            return vector2ToString(value.vector2Value);
        case ScriptValueType::Prefab:
        case ScriptValueType::Entity:
        case ScriptValueType::String:
        default:
            return value.stringValue;
    }
}

void scriptValueSetValueFromString(ScriptValue& scriptValue, const std::string& value) {
    switch (scriptValue.type) {
        case ScriptValueType::Int:
            try {
                scriptValue.intValue = std::stoi(value);
            } catch (const std::invalid_argument&) {
                scriptValue.intValue = 0;
            } catch (const std::out_of_range&) {
                scriptValue.intValue = 0;
            }
            break;
        case ScriptValueType::Float:
            try {
                scriptValue.floatValue = std::stof(value);
            } catch (const std::invalid_argument&) {
                scriptValue.floatValue = 0.0f;
            } catch (const std::out_of_range&) {
                scriptValue.floatValue = 0.0f;
            }
            break;
        case ScriptValueType::Bool:
            scriptValue.boolValue = value == "true" || value == "1" || value == "True" || value == "TRUE";
            break;
        case ScriptValueType::Vector2:
            scriptValue.vector2Value = parseVector2Value(value);
            break;
        case ScriptValueType::Prefab:
        case ScriptValueType::Entity:
        case ScriptValueType::String:
        default:
            scriptValue.stringValue = value;
            break;
    }
}

std::string scriptPropertyValueToString(const ScriptProperty& property) {
    switch (property.type) {
        case ScriptPropertyType::Int:
            return std::to_string(property.intValue);
        case ScriptPropertyType::Float: {
            std::ostringstream stream;
            stream << property.floatValue;
            return stream.str();
        }
        case ScriptPropertyType::Bool:
            return property.boolValue ? "true" : "false";
        case ScriptPropertyType::Vector2:
            return vector2ToString(property.vector2Value);
        case ScriptPropertyType::Array:
        case ScriptPropertyType::Map:
            return "";
        case ScriptPropertyType::Prefab:
        case ScriptPropertyType::Entity:
        case ScriptPropertyType::String:
        default:
            return property.stringValue;
    }
}

void scriptPropertySetValueFromString(ScriptProperty& property, const std::string& value) {
    switch (property.type) {
        case ScriptPropertyType::Int:
            try {
                property.intValue = std::stoi(value);
            } catch (const std::invalid_argument&) {
                property.intValue = 0;
            } catch (const std::out_of_range&) {
                property.intValue = 0;
            }
            break;
        case ScriptPropertyType::Float:
            try {
                property.floatValue = std::stof(value);
            } catch (const std::invalid_argument&) {
                property.floatValue = 0.0f;
            } catch (const std::out_of_range&) {
                property.floatValue = 0.0f;
            }
            break;
        case ScriptPropertyType::Bool:
            property.boolValue = value == "true" || value == "1" || value == "True" || value == "TRUE";
            break;
        case ScriptPropertyType::Vector2:
            property.vector2Value = parseVector2Value(value);
            break;
        case ScriptPropertyType::Array:
        case ScriptPropertyType::Map:
            break;
        case ScriptPropertyType::Prefab:
        case ScriptPropertyType::Entity:
        case ScriptPropertyType::String:
        default:
            property.stringValue = value;
            break;
    }
}

ScriptComponent::ScriptComponent(std::string scriptPath)
    : scriptPath(std::move(scriptPath)) {}

ScriptComponent::ScriptComponent(std::string scriptPath, std::vector<ScriptProperty> properties)
    : scriptPath(std::move(scriptPath)),
      properties(std::move(properties)) {}

const std::string& ScriptComponent::getScriptPath() const {
    return scriptPath;
}

bool ScriptComponent::isLoaded() const {
    return loaded;
}

const std::vector<ScriptProperty>& ScriptComponent::getProperties() const {
    return properties;
}

std::vector<ScriptProperty>& ScriptComponent::getProperties() {
    return properties;
}

ScriptProperty& ScriptComponent::addProperty(const std::string& name, ScriptPropertyType type) {
    ScriptProperty property;
    property.name = makeUniquePropertyName(properties, name);
    property.type = type;
    properties.push_back(std::move(property));
    return properties.back();
}

bool ScriptComponent::removeProperty(std::size_t index) {
    if (index >= properties.size()) {
        return false;
    }

    properties.erase(properties.begin() + static_cast<std::vector<ScriptProperty>::difference_type>(index));
    return true;
}

std::string ScriptComponent::getString(const std::string& name, const std::string& fallback) const {
    const ScriptProperty* property = findProperty(name);
    if (property == nullptr) {
        return fallback;
    }

    if (property->type == ScriptPropertyType::String || property->type == ScriptPropertyType::Prefab) {
        return property->stringValue;
    }

    return scriptPropertyValueToString(*property);
}

Entity* ScriptComponent::getEntityReference(const std::string& name) const {
    const ScriptProperty* property = findProperty(name);
    if (property == nullptr || property->type != ScriptPropertyType::Entity || property->stringValue.empty()) {
        return nullptr;
    }

    return Level::getCurrentLevel().getEntityByGuid(property->stringValue);
}

int ScriptComponent::getInt(const std::string& name, int fallback) const {
    const ScriptProperty* property = findProperty(name);
    if (property == nullptr) {
        return fallback;
    }

    if (property->type == ScriptPropertyType::Int) {
        return property->intValue;
    }

    if (property->type == ScriptPropertyType::Float) {
        return static_cast<int>(property->floatValue);
    }

    return fallback;
}

float ScriptComponent::getFloat(const std::string& name, float fallback) const {
    const ScriptProperty* property = findProperty(name);
    if (property == nullptr) {
        return fallback;
    }

    if (property->type == ScriptPropertyType::Float) {
        return property->floatValue;
    }

    if (property->type == ScriptPropertyType::Int) {
        return static_cast<float>(property->intValue);
    }

    return fallback;
}

bool ScriptComponent::getBool(const std::string& name, bool fallback) const {
    const ScriptProperty* property = findProperty(name);
    if (property == nullptr) {
        return fallback;
    }

    if (property->type == ScriptPropertyType::Bool) {
        return property->boolValue;
    }

    return fallback;
}

void ScriptComponent::onStart() {
    loaded = ScriptSystem::getInstance().loadScript(*this);
}

void ScriptComponent::onUpdate(float deltaTime) {
    if (!loaded || !this->isEnabled()) {
        return;
    }

    ScriptSystem::getInstance().updateScript(*this, deltaTime);
}

void ScriptComponent::onDestroy() {
    ScriptSystem::getInstance().unloadScript(*this);
    loaded = false;
}

std::unique_ptr<Component> ScriptComponent::clone() const {
    auto copy = std::make_unique<ScriptComponent>(scriptPath, properties);
    copy->setEnabled(isEnabled());
    return copy;
}

const ScriptProperty* ScriptComponent::findProperty(const std::string& name) const {
    for (const ScriptProperty& property : properties) {
        if (property.name == name) {
            return &property;
        }
    }

    return nullptr;
}
