#pragma once

#include "components/Component.h"

#include <cstddef>
#include <string>
#include <vector>

enum class ScriptPropertyType {
    String,
    Int,
    Float,
    Bool,
    Prefab,
    Vector2,
    Entity,
};

struct ScriptProperty {
    std::string name;
    ScriptPropertyType type = ScriptPropertyType::String;
    std::string stringValue;
    int intValue = 0;
    float floatValue = 0.0f;
    bool boolValue = false;
};

const char* scriptPropertyTypeToString(ScriptPropertyType type);
ScriptPropertyType scriptPropertyTypeFromString(const std::string& value);
std::string scriptPropertyValueToString(const ScriptProperty& property);
void scriptPropertySetValueFromString(ScriptProperty& property, const std::string& value);

class ScriptComponent : public Component {
public:
    explicit ScriptComponent(std::string scriptPath);
    ScriptComponent(std::string scriptPath, std::vector<ScriptProperty> properties);

    const std::string& getScriptPath() const;
    bool isLoaded() const;
    const std::vector<ScriptProperty>& getProperties() const;
    std::vector<ScriptProperty>& getProperties();

    ScriptProperty& addProperty(const std::string& name, ScriptPropertyType type);
    bool removeProperty(std::size_t index);

    std::string getString(const std::string& name, const std::string& fallback) const;
    int getInt(const std::string& name, int fallback) const;
    float getFloat(const std::string& name, float fallback) const;
    bool getBool(const std::string& name, bool fallback) const;

    void onStart() override;
    void onUpdate(float deltaTime) override;
    void onDestroy() override;

private:
    const ScriptProperty* findProperty(const std::string& name) const;

    std::string scriptPath;
    std::vector<ScriptProperty> properties;
    bool loaded = false;
};
