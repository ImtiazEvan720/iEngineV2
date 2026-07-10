#include "serialization/components/UIButtonComponentSerializer.h"

#include "Entity.h"
#include "components/UIButtonComponent.h"
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

const char* UIButtonComponentSerializer::getTypeName() const {
    return "UIButtonComponent";
}

bool UIButtonComponentSerializer::hasComponent(const Entity& entity) const {
    return entity.getComponent<UIButtonComponent>() != nullptr;
}

void UIButtonComponentSerializer::save(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const Entity& entity
) const {
    const UIButtonComponent* button = entity.getComponent<UIButtonComponent>();
    if (button == nullptr) {
        return;
    }

    tinyxml2::XMLElement* component =
        ComponentSerializationHelpers::addComponentElement(document, entityElement, getTypeName());
    ComponentSerializationHelpers::setComponentEnabledAttribute(*component, *button);
    component->SetAttribute("action", button->getAction().c_str());
    component->SetAttribute(
        "interactable",
        ComponentSerializationHelpers::boolText(button->isInteractable())
    );
    saveColorAttributes(*component, "normalColor", button->getNormalColor());
    saveColorAttributes(*component, "hoverColor", button->getHoverColor());
    saveColorAttributes(*component, "pressedColor", button->getPressedColor());
    saveColorAttributes(*component, "disabledColor", button->getDisabledColor());
}

bool UIButtonComponentSerializer::load(
    const tinyxml2::XMLElement& componentElement,
    Entity& entity,
    const ComponentSerializationContext& context,
    std::string& errorMessage
) const {
    (void)context;
    (void)errorMessage;

    UIButtonComponent* button = entity.getComponent<UIButtonComponent>();
    if (button == nullptr) {
        button = &entity.addComponent<UIButtonComponent>();
    }

    const char* action = componentElement.Attribute("action");
    button->setAction(action == nullptr ? "" : action);
    button->setInteractable(ComponentSerializationHelpers::parseBool(
        componentElement.Attribute("interactable"),
        true
    ));
    button->setNormalColor(readColorAttributes(componentElement, "normalColor", button->getNormalColor()));
    button->setHoverColor(readColorAttributes(componentElement, "hoverColor", button->getHoverColor()));
    button->setPressedColor(readColorAttributes(componentElement, "pressedColor", button->getPressedColor()));
    button->setDisabledColor(readColorAttributes(componentElement, "disabledColor", button->getDisabledColor()));
    ComponentSerializationHelpers::applyComponentEnabledAttribute(componentElement, *button);
    return true;
}

bool UIButtonComponentSerializer::requiresRectTransformBeforeLoad() const {
    return true;
}
