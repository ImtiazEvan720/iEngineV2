#include "editor/components/UIEditTextComponentDrawer.h"

#include "Entity.h"
#include "components/ScriptComponent.h"
#include "components/UIEditTextComponent.h"

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

bool drawColorField(
    const char* label,
    const RenderColor& currentColor,
    UIEditTextComponent& editText,
    void (UIEditTextComponent::*setter)(const RenderColor&)
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

    (editText.*setter)(toRenderColor(color));
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

void UIEditTextComponentDrawer::draw(
    Entity& entity,
    Component& component,
    std::string& statusMessage
) {
    auto* editText = dynamic_cast<UIEditTextComponent*>(&component);
    if (editText == nullptr) {
        ImGui::TextDisabled("Invalid UIEditTextComponent.");
        return;
    }

    std::string text = editText->getText();
    if (ImGui::InputText("Text", &text)) {
        editText->setText(text);
        statusMessage = "Updated UIEditTextComponent text.";
    }

    std::string placeholder = editText->getPlaceholder();
    if (ImGui::InputText("Placeholder", &placeholder)) {
        editText->setPlaceholder(placeholder);
        statusMessage = "Updated UIEditTextComponent placeholder.";
    }

    ScriptComponent* script = entity.getComponent<ScriptComponent>();
    if (script == nullptr) {
        std::string submitFunction = editText->getSubmitFunction();
        if (ImGui::InputText("Submit Function", &submitFunction)) {
            editText->setSubmitFunction(submitFunction);
            statusMessage = "Updated UIEditTextComponent submit function.";
        }
        ImGui::TextDisabled("Attach ScriptComponent to choose from Lua functions.");
    } else {
        const std::vector<std::string> functions = getLuaGlobalFunctionsFromScript(*script);
        std::string selectedFunction = editText->getSubmitFunction();

        if (functions.empty()) {
            if (ImGui::InputText("Submit Function", &selectedFunction)) {
                editText->setSubmitFunction(selectedFunction);
                statusMessage = "Updated UIEditTextComponent submit function.";
            }
            ImGui::TextDisabled("No global Lua functions found.");
        } else if (ImGui::BeginCombo("Submit Function", selectedFunction.c_str())) {
            for (const std::string& functionName : functions) {
                const bool selected = functionName == selectedFunction;
                if (ImGui::Selectable(functionName.c_str(), selected)) {
                    editText->setSubmitFunction(functionName);
                    statusMessage = "Updated UIEditTextComponent submit function.";
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }
    }

    std::string fontName = editText->getFontName();
    if (ImGui::InputText("Font", &fontName)) {
        editText->setFontName(fontName);
        statusMessage = "Updated UIEditTextComponent font.";
    }

    int fontSize = editText->getFontSize();
    ImGui::InputInt("Font Size", &fontSize);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        editText->setFontSize(fontSize);
        statusMessage = "Updated UIEditTextComponent font size.";
    }

    int maxLength = editText->getMaxLength();
    ImGui::InputInt("Max Length", &maxLength);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        editText->setMaxLength(maxLength);
        statusMessage = "Updated UIEditTextComponent max length.";
    }

    bool readOnly = editText->isReadOnly();
    if (ImGui::Checkbox("Read Only", &readOnly)) {
        editText->setReadOnly(readOnly);
        statusMessage = "Updated UIEditTextComponent read-only state.";
    }

    bool password = editText->isPassword();
    if (ImGui::Checkbox("Password", &password)) {
        editText->setPassword(password);
        statusMessage = "Updated UIEditTextComponent password state.";
    }

    ImGui::Text(
        "Runtime: focused=%s cursor=%zu",
        editText->isFocused() ? "true" : "false",
        editText->getCursorIndex()
    );

    if (drawColorField(
            "Text Color",
            editText->getTextColor(),
            *editText,
            &UIEditTextComponent::setTextColor)) {
        statusMessage = "Updated UIEditTextComponent text color.";
    }

    if (drawColorField(
            "Placeholder Color",
            editText->getPlaceholderColor(),
            *editText,
            &UIEditTextComponent::setPlaceholderColor)) {
        statusMessage = "Updated UIEditTextComponent placeholder color.";
    }

    if (drawColorField(
            "Background Color",
            editText->getBackgroundColor(),
            *editText,
            &UIEditTextComponent::setBackgroundColor)) {
        statusMessage = "Updated UIEditTextComponent background color.";
    }

    if (drawColorField(
            "Focused Color",
            editText->getFocusedColor(),
            *editText,
            &UIEditTextComponent::setFocusedColor)) {
        statusMessage = "Updated UIEditTextComponent focused color.";
    }

    if (drawColorField(
            "Cursor Color",
            editText->getCursorColor(),
            *editText,
            &UIEditTextComponent::setCursorColor)) {
        statusMessage = "Updated UIEditTextComponent cursor color.";
    }
}
