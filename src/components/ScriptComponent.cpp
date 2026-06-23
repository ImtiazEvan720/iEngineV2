#include "components/ScriptComponent.h"

#include "system/ScriptSystem.h"

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

    return ScriptPropertyType::String;
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
        case ScriptPropertyType::Prefab:
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
        case ScriptPropertyType::Prefab:
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
    if (!loaded) {
        return;
    }

    ScriptSystem::getInstance().updateScript(*this, deltaTime);
}

void ScriptComponent::onDestroy() {
    ScriptSystem::getInstance().unloadScript(*this);
    loaded = false;
}

const ScriptProperty* ScriptComponent::findProperty(const std::string& name) const {
    for (const ScriptProperty& property : properties) {
        if (property.name == name) {
            return &property;
        }
    }

    return nullptr;
}
