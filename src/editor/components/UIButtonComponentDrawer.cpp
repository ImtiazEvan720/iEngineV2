#include "editor/components/UIButtonComponentDrawer.h"

#include "Entity.h"
#include "components/ScriptComponent.h"
#include "components/UIButtonComponent.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <vector>

namespace {
std::uint8_t toColorChannel(float value) {
    const float clampedValue = std::clamp(value, 0.0f, 1.0f);
    return static_cast<std::uint8_t>((clampedValue * 255.0f) + 0.5f);
}

RenderColor toRenderColor(const float color[4]) {
    return RenderColor{
        toColorChannel(color[0]),
        toColorChannel(color[1]),
        toColorChannel(color[2]),
        toColorChannel(color[3])
    };
}

void copyRenderColorToFloats(const RenderColor& color, float outColor[4]) {
    outColor[0] = color.r / 255.0f;
    outColor[1] = color.g / 255.0f;
    outColor[2] = color.b / 255.0f;
    outColor[3] = color.a / 255.0f;
}

const char* stateToText(UIButtonState state) {
    switch (state) {
        case UIButtonState::Hovered:
            return "Hovered";
        case UIButtonState::Pressed:
            return "Pressed";
        case UIButtonState::Disabled:
            return "Disabled";
        case UIButtonState::Normal:
        default:
            return "Normal";
    }
}

bool drawColorField(
    const char* label,
    const RenderColor& currentColor,
    void (UIButtonComponent::*setter)(const RenderColor&),
    UIButtonComponent& button
) {
    float color[4];
    copyRenderColorToFloats(currentColor, color);
    if (!ImGui::ColorEdit4(
            label,
            color,
            ImGuiColorEditFlags_AlphaBar |
                ImGuiColorEditFlags_AlphaPreviewHalf |
                ImGuiColorEditFlags_DisplayRGB)) {
        return false;
    }

    (button.*setter)(toRenderColor(color));
    return true;
}

std::vector<std::string> getLuaGlobalFunctionsFromScript(const ScriptComponent& script) {
    std::vector<std::string> functions;

    std::filesystem::path scriptPath(script.getScriptPath());
    if (!std::filesystem::exists(scriptPath)) {
        scriptPath = std::filesystem::current_path() / script.getScriptPath();
    }

    std::ifstream file(scriptPath);
    if (!file.is_open()) {
        return functions;
    }

    const std::regex functionPattern(R"(^\s*function\s+([A-Za-z_][A-Za-z0-9_]*)\s*\()");
    std::string line;

    while (std::getline(file, line)) {
        std::smatch match;
        if (std::regex_search(line, match, functionPattern)) {
            functions.push_back(match[1].str());
        }
    }

    return functions;
}
}

void UIButtonComponentDrawer::draw(
    Entity& entity,
    Component& component,
    std::string& statusMessage
) {
    auto* button = dynamic_cast<UIButtonComponent*>(&component);
    if (button == nullptr) {
        ImGui::TextDisabled("Invalid UIButtonComponent.");
        return;
    }

    ScriptComponent* script = entity.getComponent<ScriptComponent>();
    if (script == nullptr) {
        ImGui::TextDisabled("No ScriptComponent attached.");
    } else {
        const std::vector<std::string> functions = getLuaGlobalFunctionsFromScript(*script);
        std::string selectedFunction = button->getAction();

        if (functions.empty()) {
            ImGui::TextDisabled("No global Lua functions found.");
        } else if (ImGui::BeginCombo("On Click Function", selectedFunction.c_str())) {
            for (const std::string& functionName : functions) {
                const bool selected = functionName == selectedFunction;

                if (ImGui::Selectable(functionName.c_str(), selected)) {
                    button->setAction(functionName);
                    statusMessage = "Updated UIButtonComponent on-click function.";
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }
    }

    bool interactable = button->isInteractable();
    if (ImGui::Checkbox("Interactable", &interactable)) {
        button->setInteractable(interactable);
        statusMessage = "Updated UIButtonComponent interactable state.";
    }

    ImGui::Text("State: %s", stateToText(button->getState()));
    ImGui::Text(
        "Runtime: hovered=%s pressed=%s clicked=%s focused=%s",
        button->isHovered() ? "true" : "false",
        button->isPressed() ? "true" : "false",
        button->wasClickedThisFrame() ? "true" : "false",
        button->isFocused() ? "true" : "false"
    );

    if (drawColorField(
            "Normal Color",
            button->getNormalColor(),
            &UIButtonComponent::setNormalColor,
            *button)) {
        statusMessage = "Updated UIButtonComponent normal color.";
    }

    if (drawColorField(
            "Hover Color",
            button->getHoverColor(),
            &UIButtonComponent::setHoverColor,
            *button)) {
        statusMessage = "Updated UIButtonComponent hover color.";
    }

    if (drawColorField(
            "Pressed Color",
            button->getPressedColor(),
            &UIButtonComponent::setPressedColor,
            *button)) {
        statusMessage = "Updated UIButtonComponent pressed color.";
    }

    if (drawColorField(
            "Disabled Color",
            button->getDisabledColor(),
            &UIButtonComponent::setDisabledColor,
            *button)) {
        statusMessage = "Updated UIButtonComponent disabled color.";
    }
}
