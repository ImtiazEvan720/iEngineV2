#include "serialization/components/UIPanelComponentSerializer.h"

#include "Entity.h"
#include "components/UIPanelComponent.h"
#include "serialization/ComponentSerializationHelpers.h"

#include "tinyxml2.h"

#include <algorithm>
#include <cstdint>

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
}

const char* UIPanelComponentSerializer::getTypeName() const {
    return "UIPanelComponent";
}

bool UIPanelComponentSerializer::hasComponent(const Entity& entity) const {
    return entity.getComponent<UIPanelComponent>() != nullptr;
}

void UIPanelComponentSerializer::save(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const Entity& entity
) const {
    const UIPanelComponent* panel = entity.getComponent<UIPanelComponent>();
    if (panel == nullptr) {
        return;
    }

    tinyxml2::XMLElement* component =
        ComponentSerializationHelpers::addComponentElement(document, entityElement, getTypeName());
    ComponentSerializationHelpers::setComponentEnabledAttribute(*component, *panel);
    const RenderColor& color = panel->getColor();
    component->SetAttribute("opacity", panel->getOpacity());
    component->SetAttribute("colorR", static_cast<int>(color.r));
    component->SetAttribute("colorG", static_cast<int>(color.g));
    component->SetAttribute("colorB", static_cast<int>(color.b));
    component->SetAttribute("colorA", static_cast<int>(color.a));
}

bool UIPanelComponentSerializer::load(
    const tinyxml2::XMLElement& componentElement,
    Entity& entity,
    const ComponentSerializationContext& context,
    std::string& errorMessage
) const {
    (void)context;
    (void)errorMessage;

    UIPanelComponent* panel = entity.getComponent<UIPanelComponent>();
    if (panel == nullptr) {
        panel = &entity.addComponent<UIPanelComponent>();
    }

    RenderColor color = panel->getColor();
    color.r = readColorAttribute(componentElement, "colorR", color.r);
    color.g = readColorAttribute(componentElement, "colorG", color.g);
    color.b = readColorAttribute(componentElement, "colorB", color.b);
    color.a = readColorAttribute(componentElement, "colorA", color.a);
    panel->setColor(color);
    panel->setOpacity(componentElement.FloatAttribute("opacity", 1.0f));
    ComponentSerializationHelpers::applyComponentEnabledAttribute(componentElement, *panel);
    return true;
}

bool UIPanelComponentSerializer::requiresRectTransformBeforeLoad() const {
    return true;
}
