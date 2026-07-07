#include "serialization/ComponentSerializerRegistry.h"

#include "Entity.h"
#include "components/RectTransformComponent.h"
#include "components/TransformComponent.h"
#include "serialization/components/AnimationComponentSerializer.h"
#include "serialization/components/CanvasComponentSerializer.h"
#include "serialization/components/CollisionComponentSerializer.h"
#include "serialization/components/GameComponentSerializers.h"
#include "serialization/components/PlayerCameraComponentSerializer.h"
#include "serialization/components/RectTransformComponentSerializer.h"
#include "serialization/components/ScriptComponentSerializer.h"
#include "serialization/components/SpriteComponentSerializer.h"
#include "serialization/components/TransformComponentSerializer.h"

#include "tinyxml2.h"

#include <iostream>

ComponentSerializerRegistry& ComponentSerializerRegistry::getInstance() {
    static ComponentSerializerRegistry registry;
    return registry;
}

ComponentSerializerRegistry::ComponentSerializerRegistry() {
    registerSerializer(std::make_unique<TransformComponentSerializer>());
    registerSerializer(std::make_unique<RectTransformComponentSerializer>());
    registerSerializer(std::make_unique<CanvasComponentSerializer>());
    registerSerializer(std::make_unique<SpriteComponentSerializer>());
    registerSerializer(std::make_unique<AnimationComponentSerializer>());
    registerSerializer(std::make_unique<CollisionComponentSerializer>());
    registerSerializer(std::make_unique<PlayerCameraComponentSerializer>());
    registerSerializer(std::make_unique<ScriptComponentSerializer>());
    registerSerializer(std::make_unique<PlayerControllerComponentSerializer>());
}

void ComponentSerializerRegistry::registerSerializer(std::unique_ptr<IComponentSerializer> serializer) {
    serializers.push_back(std::move(serializer));
}

const IComponentSerializer* ComponentSerializerRegistry::findSerializer(const std::string& typeName) const {
    for (const auto& serializer : serializers) {
        if (serializer->getTypeName() == typeName) {
            return serializer.get();
        }
    }

    return nullptr;
}

void ComponentSerializerRegistry::saveComponents(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const Entity& entity
) const {
    for (const auto& serializer : serializers) {
        if (serializer->hasComponent(entity)) {
            serializer->save(document, entityElement, entity);
        }
    }
}

bool ComponentSerializerRegistry::loadComponents(
    const tinyxml2::XMLElement& entityElement,
    Entity& entity,
    const ComponentSerializationContext& context,
    std::string& errorMessage
) const {
    const bool needsWorldRootTransform =
        context.isRootEntity && context.placement == ComponentSerializationPlacement::World;
    const bool needsRequiredComponentTransform = serializedComponentsRequireTransform(entityElement);
    const bool needsScreenRootRectTransform =
        context.isRootEntity && context.placement == ComponentSerializationPlacement::Screen;
    const bool needsRequiredComponentRectTransform = serializedComponentsRequireRectTransform(entityElement);

    if (!hasSerializedComponent(entityElement, "TransformComponent")
        && (needsWorldRootTransform || needsRequiredComponentTransform)) {
        entity.addComponent<TransformComponent>(
            needsWorldRootTransform ? context.rootPosition : Vector2F::zero(),
            0.0f
        );
    }

    if (!hasSerializedComponent(entityElement, "RectTransformComponent")
        && (needsScreenRootRectTransform || needsRequiredComponentRectTransform)) {
        entity.addComponent<RectTransformComponent>(
            needsScreenRootRectTransform ? context.rootPosition : Vector2F::zero(),
            Vector2F(100.0f, 100.0f),
            Vector2F(0.5f, 0.5f),
            0.0f
        );
    }

    for (const bool loadBeforeOtherComponents : {true, false}) {
        for (const tinyxml2::XMLElement* component = entityElement.FirstChildElement("component");
             component != nullptr;
             component = component->NextSiblingElement("component")) {
            const char* type = component->Attribute("type");
            if (type == nullptr) {
                continue;
            }

            const IComponentSerializer* serializer = findSerializer(type);
            if (serializer == nullptr) {
                std::cerr << "Unknown serialized component: " << type << std::endl;
                continue;
            }

            if (serializer->loadBeforeOtherComponents() != loadBeforeOtherComponents) {
                continue;
            }

            if (!serializer->load(*component, entity, context, errorMessage)) {
                return false;
            }
        }
    }

    return true;
}

bool ComponentSerializerRegistry::hasSerializedComponent(
    const tinyxml2::XMLElement& entityElement,
    const char* typeName
) const {
    for (const tinyxml2::XMLElement* component = entityElement.FirstChildElement("component");
         component != nullptr;
         component = component->NextSiblingElement("component")) {
        const char* type = component->Attribute("type");
        if (type != nullptr && std::string(type) == typeName) {
            return true;
        }
    }

    return false;
}

bool ComponentSerializerRegistry::serializedComponentsRequireTransform(
    const tinyxml2::XMLElement& entityElement
) const {
    for (const tinyxml2::XMLElement* component = entityElement.FirstChildElement("component");
         component != nullptr;
         component = component->NextSiblingElement("component")) {
        const char* type = component->Attribute("type");
        if (type == nullptr) {
            continue;
        }

        const IComponentSerializer* serializer = findSerializer(type);
        if (serializer != nullptr && serializer->requiresTransformBeforeLoad()) {
            return true;
        }
    }

    return false;
}

bool ComponentSerializerRegistry::serializedComponentsRequireRectTransform(
    const tinyxml2::XMLElement& entityElement
) const {
    for (const tinyxml2::XMLElement* component = entityElement.FirstChildElement("component");
         component != nullptr;
         component = component->NextSiblingElement("component")) {
        const char* type = component->Attribute("type");
        if (type == nullptr) {
            continue;
        }

        const IComponentSerializer* serializer = findSerializer(type);
        if (serializer != nullptr && serializer->requiresRectTransformBeforeLoad()) {
            return true;
        }
    }

    return false;
}
