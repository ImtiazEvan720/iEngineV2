#pragma once

#include "components/ScriptComponent.h"

#include <string>
#include <vector>

class ScriptPropertyParser {
public:
    static std::vector<ScriptProperty> parseScriptText(const std::string& scriptText);
    static std::vector<ScriptProperty> makeDefaultScriptProperties(const std::string& scriptPath);
    static void syncWithDefinitions(ScriptComponent& scriptComponent);
};
