#include "system/InputActionRegistry.h"

#include "tinyxml2.h"

#include <string>

#include <vector>

#include <iostream>

namespace {
const std::string EmptyActionName;
const char* InputValueTypeString[] =  {
    "Axis",
    "Button"
};

}

InputActionRegistry& InputActionRegistry::getInstance() {
    static InputActionRegistry instance;
    return instance;
}

void InputActionRegistry::clear() {
    actions.clear();
    idsByName.clear();
    indexesById.clear();
}

void InputActionRegistry::registerRuntimeDefaults() {
    clear();

    std::string errorMessage;
    registerAction({1, "Move", InputValueType::Axis}, errorMessage);
    registerAction({2, "Fire", InputValueType::Button}, errorMessage);
    registerAction({3, "Fire2", InputValueType::Button}, errorMessage);
    registerAction({4, "Pause", InputValueType::Button}, errorMessage);
}

bool InputActionRegistry::loadFromFile(const std::string& path, std::string& errorMessage) {
    tinyxml2::XMLDocument document;
    if (document.LoadFile(path.c_str()) != tinyxml2::XML_SUCCESS) {
        errorMessage = "Failed to load input actions: " + std::string(document.ErrorStr());
        return false;
    }

    const tinyxml2::XMLElement* root = document.FirstChildElement("inputActions");
    if (root == nullptr) {
        errorMessage = "Input actions file is missing <inputActions> root.";
        return false;
    }

    clear();

    for (const tinyxml2::XMLElement* actionElement = root->FirstChildElement("action");
         actionElement != nullptr;
         actionElement = actionElement->NextSiblingElement("action")) {
        int idValue = 0;
        if (actionElement->QueryIntAttribute("id", &idValue) != tinyxml2::XML_SUCCESS || idValue <= 0) {
            errorMessage = "Input action has missing or invalid positive id.";
            clear();
            return false;
        }

        const char* nameText = actionElement->Attribute("name");
        if (nameText == nullptr || std::string(nameText).empty()) {
            errorMessage = "Input action id " + std::to_string(idValue) + " has missing name.";
            clear();
            return false;
        }

        const char* typeText = actionElement->Attribute("type");
        if (typeText == nullptr) {
            errorMessage = "Input action " + std::string(nameText) + " has missing type.";
            clear();
            return false;
        }

        InputValueType type = InputValueType::Button;
        if (!inputValueTypeFromString(typeText, type)) {
            errorMessage = "Input action " + std::string(nameText)
                + " has unknown type: " + typeText;
            clear();
            return false;
        }

        std::string registerError;
        if (!registerAction(
                InputActionInfo{
                    static_cast<InputActionId>(idValue),
                    nameText,
                    type
                },
                registerError
            )) {
            errorMessage = registerError;
            clear();
            return false;
        }
    }

    if (actions.empty()) {
        errorMessage = "Input actions file has no actions.";
        return false;
    }

    errorMessage.clear();
    printActions(actions);
    return true;
}

bool InputActionRegistry::registerAction(
    const InputActionInfo& action,
    std::string& errorMessage
) {
    if (action.id == 0) {
        errorMessage = "Input action id cannot be 0.";
        return false;
    }

    if (action.name.empty()) {
        errorMessage = "Input action name cannot be empty.";
        return false;
    }

    if (idsByName.find(action.name) != idsByName.end()) {
        errorMessage = "Duplicate input action name: " + action.name;
        return false;
    }

    if (indexesById.find(action.id) != indexesById.end()) {
        errorMessage = "Duplicate input action id: " + std::to_string(action.id);
        return false;
    }

    indexesById[action.id] = actions.size();
    idsByName[action.name] = action.id;
    actions.push_back(action);

    errorMessage.clear();
    return true;
}

InputActionId InputActionRegistry::getActionIdByName(const std::string& name) const {
    const auto iterator = idsByName.find(name);
    if (iterator == idsByName.end()) {
        return 0;
    }

    return iterator->second;
}

const InputActionInfo* InputActionRegistry::getActionById(InputActionId id) const {
    const auto iterator = indexesById.find(id);
    if (iterator == indexesById.end()) {
        return nullptr;
    }

    return &actions[iterator->second];
}

const InputActionInfo* InputActionRegistry::getActionByName(const std::string& name) const {
    return getActionById(getActionIdByName(name));
}

const std::string& InputActionRegistry::getActionNameById(InputActionId id) const {
    const InputActionInfo* action = getActionById(id);
    if (action == nullptr) {
        return EmptyActionName;
    }

    return action->name;
}

const std::vector<InputActionInfo>& InputActionRegistry::getActions() const {
    return actions;
}

bool inputValueTypeFromString(const std::string& value, InputValueType& output) {
    if (value == "Button") {
        output = InputValueType::Button;
        return true;
    }

    if (value == "Axis") {
        output = InputValueType::Axis;
        return true;
    }

    return false;
}

const char* inputValueTypeToString(InputValueType value) {
    switch (value) {
        case InputValueType::Axis:
            return "Axis";
        case InputValueType::Button:
        default:
            return "Button";
    }
}

void printActions(const std::vector<InputActionInfo>& allActions)
{

    for(const auto& info:allActions)
    {
        int type = static_cast<int>(info.type);

        std::cout << info.name << " -- "
         << info.id << " -- "
         << InputValueTypeString[type] << std::endl;
    }
}
