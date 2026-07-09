#include "editor/components/UIPanelComponentDrawer.h"

#include "components/UIPanelComponent.h"

#include "imgui.h"

#include <algorithm>
#include <cstdint>

namespace {
std::uint8_t toColorChannel(float value) {
    return static_cast<std::uint8_t>(
        (std::clamp(value, 0.0f, 1.0f) * 255.0f) + 0.5f
    );
}
}

void UIPanelComponentDrawer::draw(
    Entity& entity,
    Component& component,
    std::string& statusMessage
) {
    (void)entity;

    auto* panel = dynamic_cast<UIPanelComponent*>(&component);
    if (panel == nullptr) {
        ImGui::TextDisabled("Invalid UIPanelComponent.");
        return;
    }

    const RenderColor currentColor = panel->getColor();
    float color[4] = {
        currentColor.r / 255.0f,
        currentColor.g / 255.0f,
        currentColor.b / 255.0f,
        currentColor.a / 255.0f
    };
    if (ImGui::ColorEdit4("Panel Color", color, ImGuiColorEditFlags_AlphaBar)) {
        panel->setColor(RenderColor{
            toColorChannel(color[0]),
            toColorChannel(color[1]),
            toColorChannel(color[2]),
            toColorChannel(color[3])
        });
        statusMessage = "Updated UIPanelComponent color.";
    }

    float opacity = panel->getOpacity();
    if (ImGui::SliderFloat("Panel Opacity", &opacity, 0.0f, 1.0f, "%.2f")) {
        panel->setOpacity(opacity);
        statusMessage = "Updated UIPanelComponent opacity.";
    }
}
