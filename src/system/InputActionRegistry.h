#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

using InputActionId = uint16_t;

enum class InputValueType : uint8_t {
    Axis,
    Button
};

struct InputActionInfo {
    InputActionId id = 0;
    std::string name;
    InputValueType type = InputValueType::Button;
};

class InputActionRegistry {
public:
    static InputActionRegistry& getInstance();

    void clear();
    void registerRuntimeDefaults();

    bool loadFromFile(const std::string& path, std::string& errorMessage);
    bool registerAction(const InputActionInfo& action, std::string& errorMessage);

    InputActionId getActionIdByName(const std::string& name) const;
    const InputActionInfo* getActionById(InputActionId id) const;
    const InputActionInfo* getActionByName(const std::string& name) const;
    const std::string& getActionNameById(InputActionId id) const;
    const std::vector<InputActionInfo>& getActions() const;

private:
    std::vector<InputActionInfo> actions;
    std::unordered_map<std::string, InputActionId> idsByName;
    std::unordered_map<InputActionId, std::size_t> indexesById;
};

bool inputValueTypeFromString(const std::string& value, InputValueType& output);
const char* inputValueTypeToString(InputValueType value);
void printActions(const std::vector<InputActionInfo>& allActions);