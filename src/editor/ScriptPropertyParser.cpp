#include "editor/ScriptPropertyParser.h"

#include "Entity.h"
#include "misc/Level.h"
#include "system/ProjectManager.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace {
Entity* findEntityByNameOrTag(Level& level, const std::string& value) {
    if (value.empty()) {
        return nullptr;
    }

    if (Entity* entity = level.getEntityByGuid(value)) {
        return entity;
    }

    for (Entity& entity : level.getEntities()) {
        if (!entity.isDestroyed() && (entity.getName() == value || entity.getTag() == value)) {
            return &entity;
        }
    }

    return nullptr;
}

bool fileExists(const std::filesystem::path& path) {
    std::error_code errorCode;
    return std::filesystem::exists(path, errorCode) && std::filesystem::is_regular_file(path, errorCode);
}

std::filesystem::path resolveLuaScriptPath(const std::string& scriptPath) {
    const std::filesystem::path directPath(scriptPath);
    if (fileExists(directPath)) {
        return directPath;
    }

    const std::string genericPath = directPath.generic_string();
    const std::string assetScriptPrefix = "Assets/Scripts/";
    const std::size_t assetScriptPosition = genericPath.find(assetScriptPrefix);
    if (assetScriptPosition != std::string::npos) {
        const std::string relativeScriptPath =
            genericPath.substr(assetScriptPosition + assetScriptPrefix.size());
        const std::filesystem::path projectScriptPath =
            ProjectManager::getInstance().getAssetsPath() / "Scripts" / relativeScriptPath;

        if (fileExists(projectScriptPath)) {
            return projectScriptPath;
        }
    }

    if (directPath.is_relative()) {
        const std::filesystem::path projectRelativePath =
            ProjectManager::getInstance().getProjectRoot() / directPath;
        if (fileExists(projectRelativePath)) {
            return projectRelativePath;
        }

        const std::filesystem::path assetsRelativePath =
            ProjectManager::getInstance().getAssetsPath() / directPath;
        if (fileExists(assetsRelativePath)) {
            return assetsRelativePath;
        }
    }

    return {};
}

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        return "";
    }

    std::ostringstream stream;
    stream << file.rdbuf();
    return stream.str();
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

std::string unquoteLuaValue(const std::string& value) {
    const std::string trimmed = trimString(value);
    if (trimmed.size() >= 2
        && ((trimmed.front() == '"' && trimmed.back() == '"')
            || (trimmed.front() == '\'' && trimmed.back() == '\''))) {
        return trimmed.substr(1, trimmed.size() - 2);
    }

    return trimmed;
}

std::string stripLuaTableBraces(const std::string& value) {
    const std::string trimmed = trimString(value);
    if (trimmed.size() >= 2 && trimmed.front() == '{' && trimmed.back() == '}') {
        return trimmed.substr(1, trimmed.size() - 2);
    }

    return trimmed;
}

std::vector<std::string> splitLuaTopLevelFields(const std::string& text) {
    std::vector<std::string> fields;
    std::size_t fieldStart = 0;
    int depth = 0;
    bool inString = false;
    char stringQuote = '\0';

    for (std::size_t index = 0; index < text.size(); ++index) {
        const char character = text[index];
        const bool escaped = index > 0 && text[index - 1] == '\\';

        if ((character == '"' || character == '\'') && !escaped) {
            if (!inString) {
                inString = true;
                stringQuote = character;
            } else if (stringQuote == character) {
                inString = false;
                stringQuote = '\0';
            }
            continue;
        }

        if (inString) {
            continue;
        }

        if (character == '{') {
            ++depth;
        } else if (character == '}') {
            --depth;
        } else if (character == ',' && depth == 0) {
            std::string field = trimString(text.substr(fieldStart, index - fieldStart));
            if (!field.empty()) {
                fields.push_back(std::move(field));
            }

            fieldStart = index + 1;
        }
    }

    std::string field = trimString(text.substr(fieldStart));
    if (!field.empty()) {
        fields.push_back(std::move(field));
    }

    return fields;
}

std::size_t findTopLevelEquals(const std::string& text) {
    int depth = 0;
    bool inString = false;
    char stringQuote = '\0';

    for (std::size_t index = 0; index < text.size(); ++index) {
        const char character = text[index];
        const bool escaped = index > 0 && text[index - 1] == '\\';

        if ((character == '"' || character == '\'') && !escaped) {
            if (!inString) {
                inString = true;
                stringQuote = character;
            } else if (stringQuote == character) {
                inString = false;
                stringQuote = '\0';
            }
            continue;
        }

        if (inString) {
            continue;
        }

        if (character == '{') {
            ++depth;
        } else if (character == '}') {
            --depth;
        } else if (character == '=' && depth == 0) {
            return index;
        }
    }

    return std::string::npos;
}

std::string normalizeLuaFieldKey(const std::string& key) {
    std::string normalized = trimString(key);
    if (normalized.size() >= 4 && normalized.front() == '[' && normalized.back() == ']') {
        normalized = trimString(normalized.substr(1, normalized.size() - 2));
    }

    return unquoteLuaValue(normalized);
}

std::string extractLuaTableValue(const std::string& tableText, const std::string& key) {
    for (const std::string& field : splitLuaTopLevelFields(tableText)) {
        const std::size_t equalsPosition = findTopLevelEquals(field);
        if (equalsPosition == std::string::npos) {
            continue;
        }

        if (normalizeLuaFieldKey(field.substr(0, equalsPosition)) == key) {
            return trimString(field.substr(equalsPosition + 1));
        }
    }

    return "";
}

std::vector<std::string> extractLuaTopLevelTables(const std::string& text) {
    std::vector<std::string> tables;
    int depth = 0;
    bool inString = false;
    char stringQuote = '\0';
    std::size_t tableStart = std::string::npos;

    for (std::size_t index = 0; index < text.size(); ++index) {
        const char character = text[index];
        const bool escaped = index > 0 && text[index - 1] == '\\';

        if ((character == '"' || character == '\'') && !escaped) {
            if (!inString) {
                inString = true;
                stringQuote = character;
            } else if (stringQuote == character) {
                inString = false;
                stringQuote = '\0';
            }
            continue;
        }

        if (inString) {
            continue;
        }

        if (character == '{') {
            if (depth == 0) {
                tableStart = index + 1;
            }

            ++depth;
        } else if (character == '}') {
            --depth;
            if (depth == 0 && tableStart != std::string::npos) {
                tables.push_back(text.substr(tableStart, index - tableStart));
                tableStart = std::string::npos;
            }
        }
    }

    return tables;
}

std::string luaVector2ValueToString(const std::string& value) {
    const std::string trimmed = trimString(value);
    if (trimmed.empty()) {
        return "0,0";
    }

    if (trimmed.front() == '{' && trimmed.back() == '}') {
        const std::string tableText = stripLuaTableBraces(trimmed);
        std::string xValue = extractLuaTableValue(tableText, "x");
        std::string yValue = extractLuaTableValue(tableText, "y");
        if (xValue.empty() || yValue.empty()) {
            const std::vector<std::string> fields = splitLuaTopLevelFields(tableText);
            if (fields.size() >= 2) {
                xValue = fields[0];
                yValue = fields[1];
            }
        }

        return unquoteLuaValue(xValue.empty() ? "0" : xValue)
            + ","
            + unquoteLuaValue(yValue.empty() ? "0" : yValue);
    }

    return unquoteLuaValue(trimmed);
}

ScriptValue makeScriptValueFromLuaValue(ScriptValueType type, const std::string& luaValue) {
    ScriptValue value;
    value.type = type;
    scriptValueSetValueFromString(
        value,
        type == ScriptValueType::Vector2
            ? luaVector2ValueToString(luaValue)
            : unquoteLuaValue(luaValue)
    );
    return value;
}

std::vector<ScriptValue> parseLuaArrayDefault(const std::string& luaValue, ScriptValueType elementType) {
    std::vector<ScriptValue> values;
    const std::string tableText = stripLuaTableBraces(luaValue);
    if (tableText.empty()) {
        return values;
    }

    for (const std::string& field : splitLuaTopLevelFields(tableText)) {
        values.push_back(makeScriptValueFromLuaValue(elementType, field));
    }

    return values;
}

std::vector<ScriptMapEntry> parseLuaMapDefault(const std::string& luaValue, ScriptValueType valueType) {
    std::vector<ScriptMapEntry> values;
    const std::string tableText = stripLuaTableBraces(luaValue);
    if (tableText.empty()) {
        return values;
    }

    for (const std::string& field : splitLuaTopLevelFields(tableText)) {
        const std::size_t equalsPosition = findTopLevelEquals(field);
        if (equalsPosition == std::string::npos) {
            continue;
        }

        ScriptMapEntry entry;
        entry.key = normalizeLuaFieldKey(field.substr(0, equalsPosition));
        entry.value = makeScriptValueFromLuaValue(valueType, field.substr(equalsPosition + 1));
        values.push_back(std::move(entry));
    }

    return values;
}

std::string extractScriptPropertiesBlock(const std::string& scriptText) {
    const std::size_t propertiesPosition = scriptText.find("ScriptProperties");
    if (propertiesPosition == std::string::npos) {
        return "";
    }

    const std::size_t blockStart = scriptText.find('{', propertiesPosition);
    if (blockStart == std::string::npos) {
        return "";
    }

    int depth = 0;
    bool inString = false;
    char stringQuote = '\0';
    for (std::size_t index = blockStart; index < scriptText.size(); ++index) {
        const char character = scriptText[index];
        const bool escaped = index > 0 && scriptText[index - 1] == '\\';

        if ((character == '"' || character == '\'') && !escaped) {
            if (!inString) {
                inString = true;
                stringQuote = character;
            } else if (stringQuote == character) {
                inString = false;
                stringQuote = '\0';
            }
            continue;
        }

        if (inString) {
            continue;
        }

        if (character == '{') {
            ++depth;
        } else if (character == '}') {
            --depth;
            if (depth == 0) {
                return scriptText.substr(blockStart + 1, index - blockStart - 1);
            }
        }
    }

    return "";
}

ScriptProperty* findScriptProperty(std::vector<ScriptProperty>& properties, const std::string& name) {
    for (ScriptProperty& property : properties) {
        if (property.name == name) {
            return &property;
        }
    }

    return nullptr;
}

void applyScriptPropertyDefinition(
    ScriptProperty& property,
    const ScriptProperty& definition
) {
    if (property.type == definition.type
        && property.elementType == definition.elementType
        && property.mapValueType == definition.mapValueType) {
        return;
    }

    const std::string currentValue = scriptPropertyValueToString(property);
    const std::string fallbackValue = scriptPropertyValueToString(definition);
    const std::string nextValue = currentValue.empty() ? fallbackValue : currentValue;

    if (definition.type == ScriptPropertyType::Array || definition.type == ScriptPropertyType::Map) {
        property = definition;
        return;
    }

    property.type = definition.type;
    property.elementType = definition.elementType;
    property.mapValueType = definition.mapValueType;

    if (definition.type == ScriptPropertyType::Entity) {
        Entity* referencedEntity = findEntityByNameOrTag(Level::getCurrentLevel(), nextValue);
        property.stringValue = referencedEntity == nullptr ? nextValue : referencedEntity->getGuid();
        return;
    }

    scriptPropertySetValueFromString(property, nextValue);
}
}

std::vector<ScriptProperty> ScriptPropertyParser::parseScriptText(const std::string& scriptText) {
    std::vector<ScriptProperty> properties;
    const std::string blockText = extractScriptPropertiesBlock(scriptText);
    if (blockText.empty()) {
        return properties;
    }

    for (const std::string& tableText : extractLuaTopLevelTables(blockText)) {
        const std::string name = extractLuaTableValue(tableText, "name");
        if (name.empty()) {
            continue;
        }

        ScriptProperty property;
        property.name = unquoteLuaValue(name);
        property.type = scriptPropertyTypeFromString(unquoteLuaValue(extractLuaTableValue(tableText, "type")));
        const std::string defaultValue = extractLuaTableValue(tableText, "default");

        switch (property.type) {
            case ScriptPropertyType::Array:
                property.elementType = scriptValueTypeFromString(
                    unquoteLuaValue(extractLuaTableValue(tableText, "elementType"))
                );
                property.arrayValue = parseLuaArrayDefault(defaultValue, property.elementType);
                break;
            case ScriptPropertyType::Map:
                property.mapValueType = scriptValueTypeFromString(
                    unquoteLuaValue(extractLuaTableValue(tableText, "valueType"))
                );
                property.mapValue = parseLuaMapDefault(defaultValue, property.mapValueType);
                break;
            case ScriptPropertyType::Vector2:
                scriptPropertySetValueFromString(property, luaVector2ValueToString(defaultValue));
                break;
            default:
                scriptPropertySetValueFromString(property, unquoteLuaValue(defaultValue));
                break;
        }

        properties.push_back(std::move(property));
    }

    return properties;
}

std::vector<ScriptProperty> ScriptPropertyParser::makeDefaultScriptProperties(const std::string& scriptPath) {
    const std::filesystem::path resolvedScriptPath = resolveLuaScriptPath(scriptPath);
    if (resolvedScriptPath.empty()) {
        return {};
    }

    return parseScriptText(readTextFile(resolvedScriptPath));
}

void ScriptPropertyParser::syncWithDefinitions(ScriptComponent& scriptComponent) {
    std::vector<ScriptProperty>& properties = scriptComponent.getProperties();
    const std::vector<ScriptProperty> definitions = makeDefaultScriptProperties(scriptComponent.getScriptPath());

    for (const ScriptProperty& definition : definitions) {
        ScriptProperty* property = findScriptProperty(properties, definition.name);
        if (property == nullptr) {
            properties.push_back(definition);
            continue;
        }

        applyScriptPropertyDefinition(*property, definition);
    }
}
