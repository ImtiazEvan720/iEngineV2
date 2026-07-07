#include "serialization/components/CanvasComponentSerializer.h"

#include "Entity.h"
#include "components/CanvasComponent.h"
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

const char* CanvasComponentSerializer::getTypeName() const {
    return "CanvasComponent";
}

bool CanvasComponentSerializer::hasComponent(const Entity& entity) const {
    return entity.getComponent<CanvasComponent>() != nullptr;
}

void CanvasComponentSerializer::save(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const Entity& entity
) const {
    const CanvasComponent* canvas = entity.getComponent<CanvasComponent>();
    if (canvas == nullptr) {
        return;
    }

    tinyxml2::XMLElement* component =
        ComponentSerializationHelpers::addComponentElement(document, entityElement, getTypeName());
    ComponentSerializationHelpers::setComponentEnabledAttribute(*component, *canvas);

    const Vector2F& referenceResolution = canvas->getReferenceResolution();
    const RenderColor& canvasColor = canvas->getCanvasColor();

    component->SetAttribute("referenceWidth", referenceResolution.x);
    component->SetAttribute("referenceHeight", referenceResolution.y);
    component->SetAttribute("sortingOrder", canvas->getSortingOrder());
    component->SetAttribute("opacity", canvas->getOpacity());
    component->SetAttribute("scale", canvas->getScale());
    component->SetAttribute("colorR", static_cast<int>(canvasColor.r));
    component->SetAttribute("colorG", static_cast<int>(canvasColor.g));
    component->SetAttribute("colorB", static_cast<int>(canvasColor.b));
    component->SetAttribute("colorA", static_cast<int>(canvasColor.a));
}

bool CanvasComponentSerializer::load(
    const tinyxml2::XMLElement& componentElement,
    Entity& entity,
    const ComponentSerializationContext& context,
    std::string& errorMessage
) const {
    (void)context;
    (void)errorMessage;

    CanvasComponent* canvas = entity.getComponent<CanvasComponent>();
    if (canvas == nullptr) {
        canvas = &entity.addComponent<CanvasComponent>();
    }

    canvas->setReferenceResolution(Vector2F(
        componentElement.FloatAttribute("referenceWidth", 640.0f),
        componentElement.FloatAttribute("referenceHeight", 320.0f)
    ));
    canvas->setSortingOrder(componentElement.IntAttribute("sortingOrder", 0));
    canvas->setOpacity(componentElement.FloatAttribute("opacity", 1.0f));
    canvas->setScale(componentElement.FloatAttribute("scale", 1.0f));

    RenderColor canvasColor = canvas->getCanvasColor();
    canvasColor.r = readColorAttribute(componentElement, "colorR", canvasColor.r);
    canvasColor.g = readColorAttribute(componentElement, "colorG", canvasColor.g);
    canvasColor.b = readColorAttribute(componentElement, "colorB", canvasColor.b);
    canvasColor.a = readColorAttribute(componentElement, "colorA", canvasColor.a);
    canvas->setCanvasColor(canvasColor);

    ComponentSerializationHelpers::applyComponentEnabledAttribute(componentElement, *canvas);
    return true;
}

bool CanvasComponentSerializer::requiresRectTransformBeforeLoad() const {
    return true;
}
