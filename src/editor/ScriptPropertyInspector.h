#pragma once

#include <string>

class ScriptComponent;

class ScriptPropertyInspector {
public:
    void draw(ScriptComponent& scriptComponent, std::string& statusMessage);
};
