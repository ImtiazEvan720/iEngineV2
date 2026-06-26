#pragma once

#include "components/Component.h"
#include "math/Vector2F.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class Entity;

enum class ScriptPropertyType {
    String,
    Int,
    Float,
    Bool,
    Prefab,
    Vector2,
    Entity,
    Array,
    Map,
};

enum class ScriptValueType {
    String,
    Int,
    Float,
    Bool,
    Prefab,
    Vector2,
    Entity,
};

enum class ScriptEntityReferenceScope {
    Level,
    PrefabLocal,
};

struct ScriptValue {
    ScriptValueType type = ScriptValueType::String;
    std::string stringValue;
    int intValue = 0;
    float floatValue = 0.0f;
    bool boolValue = false;
    Vector2F vector2Value = Vector2F::zero();
    ScriptEntityReferenceScope entityReferenceScope = ScriptEntityReferenceScope::Level;
};

struct ScriptMapEntry {
    std::string key;
    ScriptValue value;
};

struct ScriptProperty {
    std::string name;
    ScriptPropertyType type = ScriptPropertyType::String;
    std::string stringValue;
    int intValue = 0;
    float floatValue = 0.0f;
    bool boolValue = false;
    Vector2F vector2Value = Vector2F::zero();
    ScriptValueType elementType = ScriptValueType::String;
    std::vector<ScriptValue> arrayValue;
    ScriptValueType mapValueType = ScriptValueType::String;
    std::vector<ScriptMapEntry> mapValue;
    ScriptEntityReferenceScope entityReferenceScope = ScriptEntityReferenceScope::Level;
};

const char* scriptPropertyTypeToString(ScriptPropertyType type);
ScriptPropertyType scriptPropertyTypeFromString(const std::string& value);
const char* scriptValueTypeToString(ScriptValueType type);
ScriptValueType scriptValueTypeFromString(const std::string& value);
const char* scriptEntityReferenceScopeToString(ScriptEntityReferenceScope scope);
ScriptEntityReferenceScope scriptEntityReferenceScopeFromString(const std::string& value);
std::string scriptValueToString(const ScriptValue& value);
void scriptValueSetValueFromString(ScriptValue& scriptValue, const std::string& value);
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
    Entity* getEntityReference(const std::string& name) const;
    int getInt(const std::string& name, int fallback) const;
    float getFloat(const std::string& name, float fallback) const;
    bool getBool(const std::string& name, bool fallback) const;

    void onStart() override;
    void onUpdate(float deltaTime) override;
    void onDestroy() override;
    std::unique_ptr<Component> clone() const override;

private:
    const ScriptProperty* findProperty(const std::string& name) const;

    std::string scriptPath;
    std::vector<ScriptProperty> properties;
    bool loaded = false;
};
