#include "editor/components/ScriptComponentDrawer.h"

#include "components/ScriptComponent.h"

#include "imgui.h"

void ScriptComponentDrawer::draw(
    Entity& entity,
    Component& component,
    std::string& statusMessage
) {
    (void)entity;

    auto* scriptComponent = dynamic_cast<ScriptComponent*>(&component);
    if (scriptComponent == nullptr) {
        ImGui::TextDisabled("Invalid ScriptComponent.");
        return;
    }

    scriptPropertyInspector.draw(*scriptComponent, statusMessage);
}
