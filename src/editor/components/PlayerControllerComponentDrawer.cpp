#include "editor/components/PlayerControllerComponentDrawer.h"

#include "components/PlayerController.h"

#include "imgui.h"

void PlayerControllerComponentDrawer::draw(
    Entity& entity,
    Component& component,
    std::string& statusMessage
) {
    (void)entity;
    (void)statusMessage;

    auto* playerController = dynamic_cast<PlayerController*>(&component);
    if (playerController == nullptr) {
        ImGui::TextDisabled("Invalid PlayerController.");
        return;
    }

    ImGui::TextUnformatted("PlayerController has no editable fields.");
}
