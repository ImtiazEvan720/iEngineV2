#include "editor/components/UILabelComponentDrawer.h"

#include "Entity.h"
#include "components/UILabelComponent.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <cstdint>

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

int toEditValue(HorizontalTextAlign value) {
    switch (value) {
        case HorizontalTextAlign::left:
            return 0;
        case HorizontalTextAlign::right:
            return 2;
        case HorizontalTextAlign::center:
        default:
            return 1;
    }
}

int toEditValue(VerticalTextAlign value) {
    switch (value) {
        case VerticalTextAlign::top:
            return 0;
        case VerticalTextAlign::bottom:
            return 2;
        case VerticalTextAlign::center:
        default:
            return 1;
    }
}

HorizontalTextAlign toHorizontalAlign(int value) {
    switch (value) {
        case 0:
            return HorizontalTextAlign::left;
        case 2:
            return HorizontalTextAlign::right;
        case 1:
        default:
            return HorizontalTextAlign::center;
    }
}

VerticalTextAlign toVerticalAlign(int value) {
    switch (value) {
        case 0:
            return VerticalTextAlign::top;
        case 2:
            return VerticalTextAlign::bottom;
        case 1:
        default:
            return VerticalTextAlign::center;
    }
}
}

void UILabelComponentDrawer::draw(
    Entity& entity,
    Component& component,
    std::string& statusMessage
) {
    auto* label = dynamic_cast<UILabelComponent*>(&component);
    if (label == nullptr) {
        ImGui::TextDisabled("Invalid UILabelComponent.");
        return;
    }

    if (editState.entityId != entity.getId()) {
        editState.entityId = entity.getId();
        editState.text = label->getText();
        editState.fontName = label->getFontName();
        editState.fontSize = label->getFontSize();
        editState.fontColor = label->getFontColor();
        editState.horizontalAlign = toEditValue(label->getHorizontalTextAlign());
        editState.verticalAlign = toEditValue(label->getVerticalTextAlign());
    }

    if (ImGui::InputText("Text", &editState.text)) {
        label->setText(editState.text);
        statusMessage = "Updated UILabelComponent text.";
    }

    if (ImGui::InputText("Font", &editState.fontName)) {
        label->setFont(editState.fontName);
        statusMessage = "Updated UILabelComponent font.";
    }

    ImGui::InputInt("Font Size", &editState.fontSize);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        editState.fontSize = std::max(1, editState.fontSize);
        label->setFontSize(editState.fontSize);
        statusMessage = "Updated UILabelComponent font size.";
    }

    float color[4];
    copyRenderColorToFloats(editState.fontColor, color);
    if (ImGui::ColorEdit4(
            "Font Color",
            color,
            ImGuiColorEditFlags_AlphaBar |
                ImGuiColorEditFlags_AlphaPreviewHalf |
                ImGuiColorEditFlags_DisplayRGB)) {
        editState.fontColor = toRenderColor(color);
        label->setFontColor(editState.fontColor);
        statusMessage = "Updated UILabelComponent font color.";
    }

    static const char* horizontalItems[] = {"Left", "Center", "Right"};
    if (ImGui::Combo("Horizontal Align", &editState.horizontalAlign, horizontalItems, 3)) {
        label->setHorizontalTextAlign(toHorizontalAlign(editState.horizontalAlign));
        statusMessage = "Updated UILabelComponent horizontal alignment.";
    }

    static const char* verticalItems[] = {"Top", "Center", "Bottom"};
    if (ImGui::Combo("Vertical Align", &editState.verticalAlign, verticalItems, 3)) {
        label->setVerticalTextAlign(toVerticalAlign(editState.verticalAlign));
        statusMessage = "Updated UILabelComponent vertical alignment.";
    }
}
