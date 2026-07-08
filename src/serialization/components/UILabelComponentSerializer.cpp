#include "serialization/components/UILabelComponentSerializer.h"

#include "Entity.h"
#include "components/UILabelComponent.h"
#include "serialization/ComponentSerializationHelpers.h"

#include "tinyxml2.h"

#include <algorithm>
#include <cstdint>
#include <string>

namespace {
std::uint8_t readColorAttribute(
    const tinyxml2::XMLElement& element,
    const char* name,
    std::uint8_t fallback
) {
    return static_cast<std::uint8_t>(std::clamp(
        element.IntAttribute(name, static_cast<int>(fallback)),
        0,
        255
    ));
}

const char* toString(HorizontalTextAlign value) {
    switch (value) {
        case HorizontalTextAlign::left:
            return "left";
        case HorizontalTextAlign::right:
            return "right";
        case HorizontalTextAlign::center:
        default:
            return "center";
    }
}

const char* toString(VerticalTextAlign value) {
    switch (value) {
        case VerticalTextAlign::top:
            return "top";
        case VerticalTextAlign::bottom:
            return "bottom";
        case VerticalTextAlign::center:
        default:
            return "center";
    }
}

HorizontalTextAlign parseHorizontalAlign(const char* value) {
    if (value == nullptr) {
        return HorizontalTextAlign::center;
    }

    const std::string text(value);
    if (text == "left") {
        return HorizontalTextAlign::left;
    }

    if (text == "right") {
        return HorizontalTextAlign::right;
    }

    return HorizontalTextAlign::center;
}

VerticalTextAlign parseVerticalAlign(const char* value) {
    if (value == nullptr) {
        return VerticalTextAlign::center;
    }

    const std::string text(value);
    if (text == "top") {
        return VerticalTextAlign::top;
    }

    if (text == "bottom") {
        return VerticalTextAlign::bottom;
    }

    return VerticalTextAlign::center;
}
}

const char* UILabelComponentSerializer::getTypeName() const {
    return "UILabelComponent";
}

bool UILabelComponentSerializer::hasComponent(const Entity& entity) const {
    return entity.getComponent<UILabelComponent>() != nullptr;
}

void UILabelComponentSerializer::save(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const Entity& entity
) const {
    const UILabelComponent* label = entity.getComponent<UILabelComponent>();
    if (label == nullptr) {
        return;
    }

    tinyxml2::XMLElement* component =
        ComponentSerializationHelpers::addComponentElement(document, entityElement, getTypeName());
    ComponentSerializationHelpers::setComponentEnabledAttribute(*component, *label);

    const RenderColor& fontColor = label->getFontColor();
    component->SetAttribute("text", label->getText().c_str());
    component->SetAttribute("font", label->getFontName().c_str());
    component->SetAttribute("fontSize", label->getFontSize());
    component->SetAttribute("colorR", static_cast<int>(fontColor.r));
    component->SetAttribute("colorG", static_cast<int>(fontColor.g));
    component->SetAttribute("colorB", static_cast<int>(fontColor.b));
    component->SetAttribute("colorA", static_cast<int>(fontColor.a));
    component->SetAttribute("horizontalAlign", toString(label->getHorizontalTextAlign()));
    component->SetAttribute("verticalAlign", toString(label->getVerticalTextAlign()));
}

bool UILabelComponentSerializer::load(
    const tinyxml2::XMLElement& componentElement,
    Entity& entity,
    const ComponentSerializationContext& context,
    std::string& errorMessage
) const {
    (void)context;
    (void)errorMessage;

    UILabelComponent* label = entity.getComponent<UILabelComponent>();
    if (label == nullptr) {
        label = &entity.addComponent<UILabelComponent>();
    }

    const char* text = componentElement.Attribute("text");
    label->setText(text == nullptr ? "Label" : text);

    const char* fontName = componentElement.Attribute("font");
    label->setFont(fontName == nullptr || fontName[0] == '\0'
        ? "Assets/Fonts/default.ttf"
        : fontName);
    label->setFontSize(componentElement.IntAttribute("fontSize", 18));

    RenderColor fontColor = label->getFontColor();
    fontColor.r = readColorAttribute(componentElement, "colorR", fontColor.r);
    fontColor.g = readColorAttribute(componentElement, "colorG", fontColor.g);
    fontColor.b = readColorAttribute(componentElement, "colorB", fontColor.b);
    fontColor.a = readColorAttribute(componentElement, "colorA", fontColor.a);
    label->setFontColor(fontColor);

    label->setHorizontalTextAlign(parseHorizontalAlign(componentElement.Attribute("horizontalAlign")));
    label->setVerticalTextAlign(parseVerticalAlign(componentElement.Attribute("verticalAlign")));
    ComponentSerializationHelpers::applyComponentEnabledAttribute(componentElement, *label);

    return true;
}

bool UILabelComponentSerializer::requiresRectTransformBeforeLoad() const {
    return true;
}
