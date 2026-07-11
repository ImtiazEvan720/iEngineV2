#include "serialization/components/UIEditTextComponentSerializer.h"

#include "Entity.h"
#include "components/UIEditTextComponent.h"
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

void saveColorAttributes(
    tinyxml2::XMLElement& element,
    const char* prefix,
    const RenderColor& color
) {
    element.SetAttribute((std::string(prefix) + "R").c_str(), static_cast<int>(color.r));
    element.SetAttribute((std::string(prefix) + "G").c_str(), static_cast<int>(color.g));
    element.SetAttribute((std::string(prefix) + "B").c_str(), static_cast<int>(color.b));
    element.SetAttribute((std::string(prefix) + "A").c_str(), static_cast<int>(color.a));
}

RenderColor readColorAttributes(
    const tinyxml2::XMLElement& element,
    const char* prefix,
    const RenderColor& fallback
) {
    RenderColor color = fallback;
    color.r = readColorAttribute(element, (std::string(prefix) + "R").c_str(), color.r);
    color.g = readColorAttribute(element, (std::string(prefix) + "G").c_str(), color.g);
    color.b = readColorAttribute(element, (std::string(prefix) + "B").c_str(), color.b);
    color.a = readColorAttribute(element, (std::string(prefix) + "A").c_str(), color.a);
    return color;
}
}

const char* UIEditTextComponentSerializer::getTypeName() const {
    return "UIEditTextComponent";
}

bool UIEditTextComponentSerializer::hasComponent(const Entity& entity) const {
    return entity.getComponent<UIEditTextComponent>() != nullptr;
}

void UIEditTextComponentSerializer::save(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const Entity& entity
) const {
    const UIEditTextComponent* editText = entity.getComponent<UIEditTextComponent>();
    if (editText == nullptr) {
        return;
    }

    tinyxml2::XMLElement* component =
        ComponentSerializationHelpers::addComponentElement(document, entityElement, getTypeName());
    ComponentSerializationHelpers::setComponentEnabledAttribute(*component, *editText);
    component->SetAttribute("text", editText->getText().c_str());
    component->SetAttribute("placeholder", editText->getPlaceholder().c_str());
    component->SetAttribute("submitFunction", editText->getSubmitFunction().c_str());
    component->SetAttribute("font", editText->getFontName().c_str());
    component->SetAttribute("fontSize", editText->getFontSize());
    component->SetAttribute("maxLength", editText->getMaxLength());
    component->SetAttribute("cursorIndex", static_cast<unsigned int>(editText->getCursorIndex()));
    component->SetAttribute("readOnly", ComponentSerializationHelpers::boolText(editText->isReadOnly()));
    component->SetAttribute("password", ComponentSerializationHelpers::boolText(editText->isPassword()));
    saveColorAttributes(*component, "textColor", editText->getTextColor());
    saveColorAttributes(*component, "placeholderColor", editText->getPlaceholderColor());
    saveColorAttributes(*component, "backgroundColor", editText->getBackgroundColor());
    saveColorAttributes(*component, "focusedColor", editText->getFocusedColor());
    saveColorAttributes(*component, "cursorColor", editText->getCursorColor());
}

bool UIEditTextComponentSerializer::load(
    const tinyxml2::XMLElement& componentElement,
    Entity& entity,
    const ComponentSerializationContext& context,
    std::string& errorMessage
) const {
    (void)context;
    (void)errorMessage;

    UIEditTextComponent* editText = entity.getComponent<UIEditTextComponent>();
    if (editText == nullptr) {
        editText = &entity.addComponent<UIEditTextComponent>();
    }

    editText->setMaxLength(componentElement.IntAttribute("maxLength", editText->getMaxLength()));

    const char* text = componentElement.Attribute("text");
    editText->setText(text == nullptr ? "" : text);

    const char* placeholder = componentElement.Attribute("placeholder");
    editText->setPlaceholder(placeholder == nullptr ? "Enter text..." : placeholder);

    const char* submitFunction = componentElement.Attribute("submitFunction");
    editText->setSubmitFunction(submitFunction == nullptr ? "onSubmit" : submitFunction);

    const char* font = componentElement.Attribute("font");
    editText->setFontName(font == nullptr || font[0] == '\0' ? "Assets/Fonts/default.ttf" : font);

    editText->setFontSize(componentElement.IntAttribute("fontSize", editText->getFontSize()));
    editText->setCursorIndex(componentElement.UnsignedAttribute(
        "cursorIndex",
        static_cast<unsigned int>(editText->getText().size())
    ));
    editText->setReadOnly(ComponentSerializationHelpers::parseBool(
        componentElement.Attribute("readOnly"),
        false
    ));
    editText->setPassword(ComponentSerializationHelpers::parseBool(
        componentElement.Attribute("password"),
        false
    ));
    editText->setTextColor(readColorAttributes(componentElement, "textColor", editText->getTextColor()));
    editText->setPlaceholderColor(readColorAttributes(
        componentElement,
        "placeholderColor",
        editText->getPlaceholderColor()
    ));
    editText->setBackgroundColor(readColorAttributes(
        componentElement,
        "backgroundColor",
        editText->getBackgroundColor()
    ));
    editText->setFocusedColor(readColorAttributes(
        componentElement,
        "focusedColor",
        editText->getFocusedColor()
    ));
    editText->setCursorColor(readColorAttributes(componentElement, "cursorColor", editText->getCursorColor()));
    editText->setFocused(false);
    ComponentSerializationHelpers::applyComponentEnabledAttribute(componentElement, *editText);
    return true;
}

bool UIEditTextComponentSerializer::requiresRectTransformBeforeLoad() const {
    return true;
}
