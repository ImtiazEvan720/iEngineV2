#include "serialization/components/RectTransformComponentSerializer.h"

#include "Entity.h"
#include "components/RectTransformComponent.h"
#include "misc/RectTransformLayout.h"
#include "serialization/ComponentSerializationHelpers.h"

#include "tinyxml2.h"

const char* RectTransformComponentSerializer::getTypeName() const {
    return "RectTransformComponent";
}

bool RectTransformComponentSerializer::hasComponent(const Entity& entity) const {
    return entity.getComponent<RectTransformComponent>() != nullptr;
}

void RectTransformComponentSerializer::save(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const Entity& entity
) const {
    const RectTransformComponent* rectTransform = entity.getComponent<RectTransformComponent>();
    if (rectTransform == nullptr) {
        return;
    }

    tinyxml2::XMLElement* component =
        ComponentSerializationHelpers::addComponentElement(document, entityElement, getTypeName());
    ComponentSerializationHelpers::setComponentEnabledAttribute(*component, *rectTransform);

    const Vector2F& position = rectTransform->getAnchoredPosition();
    const Vector2F& size = rectTransform->getSize();
    const Vector2F& pivot = rectTransform->getPivot();

    component->SetAttribute("anchoredX", position.x);
    component->SetAttribute("anchoredY", position.y);
    component->SetAttribute("sizeX", size.x);
    component->SetAttribute("sizeY", size.y);
    component->SetAttribute("pivotX", pivot.x);
    component->SetAttribute("pivotY", pivot.y);
    component->SetAttribute("rotation", rectTransform->getRotation());
    component->SetAttribute(
        "anchorAlignment",
        RectTransformLayout::anchorAlignmentToString(rectTransform->getAnchorAlignment())
    );
}

bool RectTransformComponentSerializer::load(
    const tinyxml2::XMLElement& componentElement,
    Entity& entity,
    const ComponentSerializationContext& context,
    std::string& errorMessage
) const {
    (void)errorMessage;

    Vector2F position(
        componentElement.FloatAttribute("anchoredX", 0.0f),
        componentElement.FloatAttribute("anchoredY", 0.0f)
    );
    if (context.isRootEntity && context.placement == ComponentSerializationPlacement::Screen) {
        position = context.rootPosition;
    }

    const Vector2F size(
        componentElement.FloatAttribute("sizeX", 0.0f),
        componentElement.FloatAttribute("sizeY", 0.0f)
    );
    const Vector2F pivot(
        componentElement.FloatAttribute("pivotX", 0.0f),
        componentElement.FloatAttribute("pivotY", 0.0f)
    );
    const float rotation = componentElement.FloatAttribute("rotation", 0.0f);
    AnchorAlignment anchorAlignment = AnchorAlignment::LeftTop;
    const char* anchorAlignmentText = componentElement.Attribute("anchorAlignment");
    if (anchorAlignmentText != nullptr) {
        (void)RectTransformLayout::anchorAlignmentFromString(anchorAlignmentText, anchorAlignment);
    }

    RectTransformComponent* rectTransform = entity.getComponent<RectTransformComponent>();
    if (rectTransform == nullptr) {
        RectTransformComponent& component = entity.addComponent<RectTransformComponent>(
            position,
            size,
            pivot,
            rotation,
            anchorAlignment
        );
        ComponentSerializationHelpers::applyComponentEnabledAttribute(componentElement, component);
    } else {
        rectTransform->setAnchoredPosition(position);
        rectTransform->setSize(size);
        rectTransform->setPivot(pivot);
        rectTransform->setRotation(rotation);
        rectTransform->setAnchorAlignment(anchorAlignment);
        ComponentSerializationHelpers::applyComponentEnabledAttribute(componentElement, *rectTransform);
    }

    return true;
}

bool RectTransformComponentSerializer::loadBeforeOtherComponents() const {
    return true;
}
