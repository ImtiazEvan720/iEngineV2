#include "serialization/components/ScriptComponentSerializer.h"

#include "Entity.h"
#include "components/ScriptComponent.h"
#include "serialization/ComponentSerializationHelpers.h"

#include "tinyxml2.h"

const char* ScriptComponentSerializer::getTypeName() const {
    return "ScriptComponent";
}

bool ScriptComponentSerializer::hasComponent(const Entity& entity) const {
    return entity.getComponent<ScriptComponent>() != nullptr;
}

void ScriptComponentSerializer::save(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const Entity& entity
) const {
    const ScriptComponent* scriptComponent = entity.getComponent<ScriptComponent>();
    if (scriptComponent == nullptr) {
        return;
    }

    tinyxml2::XMLElement* component =
        ComponentSerializationHelpers::addComponentElement(document, entityElement, getTypeName());
    component->SetAttribute("path", scriptComponent->getScriptPath().c_str());
}

bool ScriptComponentSerializer::load(
    const tinyxml2::XMLElement& componentElement,
    Entity& entity,
    const ComponentSerializationContext& context,
    std::string& errorMessage
) const {
    (void)context;
    (void)errorMessage;

    const char* scriptPath = componentElement.Attribute("path");
    if (scriptPath != nullptr && scriptPath[0] != '\0') {
        entity.addComponent<ScriptComponent>(scriptPath);
    }

    return true;
}
