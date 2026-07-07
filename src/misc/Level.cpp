#include "misc/Level.h"

#include "components/AnimationComponent.h"
#include "components/CanvasComponent.h"
#include "components/CollisionComponent.h"
#include "components/PlayerCameraComponent.h"
#include "components/PlayerController.h"
#include "components/RectTransformComponent.h"
#include "components/ScriptComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "misc/Animation.h"
#include "misc/AnimationLoader.h"
#include "misc/Guid.h"
#include "misc/Sprite.h"
#include "misc/TextureAsset.h"
#include "serialization/ComponentSerializerRegistry.h"
#include "system/AssetManager.h"
#include "system/EngineState.h"
#include "system/InputSystem.h"

#include "tinyxml2.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <iterator>

namespace
{
    std::string currentLevelPath = "Assets/Levels/current.ilevel";
    using GuidRemap = std::unordered_map<std::string, std::string>;

    bool parseBool(const char *value, bool fallback)
    {
        if (value == nullptr)
        {
            return fallback;
        }

        const std::string text = value;
        return text == "true" || text == "1" || text == "True" || text == "TRUE";
    }

    CollisionComponent::BodyType parseBodyType(const std::string &value)
    {
        if (value == "Dynamic" || value == "dynamic")
        {
            return CollisionComponent::BodyType::Dynamic;
        }

        if (value == "Kinematic" || value == "kinematic")
        {
            return CollisionComponent::BodyType::Kinematic;
        }

        return CollisionComponent::BodyType::Static;
    }

    const char *boolText(bool value)
    {
        return value ? "true" : "false";
    }

    std::uint8_t readColorAttribute(
        const tinyxml2::XMLElement &element,
        const char *name,
        std::uint8_t fallback)
    {
        return static_cast<std::uint8_t>(std::clamp(
            element.IntAttribute(name, static_cast<int>(fallback)),
            0,
            255));
    }

    const char *bodyTypeToString(CollisionComponent::BodyType bodyType)
    {
        switch (bodyType)
        {
        case CollisionComponent::BodyType::Kinematic:
            return "Kinematic";
        case CollisionComponent::BodyType::Dynamic:
            return "Dynamic";
        case CollisionComponent::BodyType::Static:
        default:
            return "Static";
        }
    }

    std::string getTextureAssetName(RenderTextureHandle textureHandle)
    {
        if (textureHandle == nullptr)
        {
            return "";
        }

        for (const auto &asset : AssetManager::getInstance().getAssets())
        {
            const auto *textureAsset = dynamic_cast<const TextureAsset *>(asset.get());
            if (textureAsset != nullptr && textureAsset->getTextureHandle() == textureHandle)
            {
                return std::filesystem::path(textureAsset->getPath()).filename().string();
            }
        }

        return "";
    }

    void remapScriptValueEntityReference(ScriptValue &value, const GuidRemap &guidRemap)
    {
        if (value.type != ScriptValueType::Entity)
        {
            return;
        }

        const auto iterator = guidRemap.find(value.stringValue);
        if (iterator != guidRemap.end())
        {
            value.stringValue = iterator->second;
        }
    }

    void remapScriptPropertyEntityReferences(ScriptProperty &property, const GuidRemap &guidRemap)
    {
        if (property.type == ScriptPropertyType::Entity)
        {
            const auto iterator = guidRemap.find(property.stringValue);
            if (iterator != guidRemap.end())
            {
                property.stringValue = iterator->second;
            }

            return;
        }

        if (property.type == ScriptPropertyType::Array && property.elementType == ScriptValueType::Entity)
        {
            for (ScriptValue &value : property.arrayValue)
            {
                remapScriptValueEntityReference(value, guidRemap);
            }

            return;
        }

        if (property.type == ScriptPropertyType::Map && property.mapValueType == ScriptValueType::Entity)
        {
            for (ScriptMapEntry &entry : property.mapValue)
            {
                remapScriptValueEntityReference(entry.value, guidRemap);
            }
        }
    }

    void remapDuplicatedScriptReferences(
        const std::vector<Entity *> &createdEntities,
        const GuidRemap &guidRemap)
    {
        for (Entity *entity : createdEntities)
        {
            if (entity == nullptr)
            {
                continue;
            }

            ScriptComponent *scriptComponent = entity->getComponent<ScriptComponent>();
            if (scriptComponent == nullptr)
            {
                continue;
            }

            for (ScriptProperty &property : scriptComponent->getProperties())
            {
                remapScriptPropertyEntityReferences(property, guidRemap);
            }
        }
    }

    Entity *duplicateEntityRecursive(
        Level &level,
        const Entity &source,
        Entity *parent,
        bool isRoot,
        bool duplicateChildren,
        GuidRemap &guidRemap,
        std::vector<Entity *> &createdEntities)
    {
        if (source.isDestroyed())
        {
            return nullptr;
        }

        Entity &copy = level.createEntity();
        copy.setComponentStartupDeferred(true);
        createdEntities.push_back(&copy);
        guidRemap[source.getGuid()] = copy.getGuid();

        copy.setName(isRoot ? source.getName() + "_Copy" : source.getName());
        copy.setTag(source.getTag());
        copy.setEnabled(source.isEnabled());
        copy.setDisplayOrder(source.getDisplayOrder() + (isRoot ? 1 : 0));
        copy.setPersistent(source.isPersistent());

        for (const std::unique_ptr<Component> &component : source.getComponents())
        {
            if (component == nullptr)
            {
                continue;
            }

            std::unique_ptr<Component> clonedComponent = component->clone();
            if (clonedComponent != nullptr)
            {
                copy.addComponent(std::move(clonedComponent));
            }
        }

        if (parent != nullptr)
        {
            copy.setParent(parent, false);
        }

        if (isRoot)
        {
            const TransformComponent *sourceTransform = source.getComponent<TransformComponent>();
            TransformComponent *copyTransform = copy.getComponent<TransformComponent>();
            if (sourceTransform != nullptr && copyTransform != nullptr)
            {
                const Vector2F sourceWorldPosition = sourceTransform->getWorldPosition();
                copyTransform->setPosition(Vector2F(
                    sourceWorldPosition.x + 32.0f,
                    sourceWorldPosition.y + 32.0f));
                copyTransform->setRotation(sourceTransform->getWorldRotation());
            }
        }

        if (!duplicateChildren)
        {
            return &copy;
        }

        for (const Entity *child : source.getChildren())
        {
            if (child != nullptr && !child->isDestroyed())
            {
                duplicateEntityRecursive(
                    level,
                    *child,
                    &copy,
                    false,
                    true,
                    guidRemap,
                    createdEntities);
            }
        }

        return &copy;
    }

    tinyxml2::XMLElement *addComponentElement(
        tinyxml2::XMLDocument &document,
        tinyxml2::XMLElement &entityElement,
        const char *type)
    {
        tinyxml2::XMLElement *component = document.NewElement("component");
        component->SetAttribute("type", type);
        entityElement.InsertEndChild(component);
        return component;
    }

    void setComponentEnabledAttribute(tinyxml2::XMLElement &element, const Component &component)
    {
        element.SetAttribute("enabled", boolText(component.isEnabled()));
    }

    void applyComponentEnabledAttribute(const tinyxml2::XMLElement &element, Component &component)
    {
        component.setEnabled(parseBool(element.Attribute("enabled"), true));
    }

    void setSpriteAttributes(tinyxml2::XMLElement &element, const Sprite &sprite)
    {
        const RenderRect &source = sprite.getSourceRect();
        const Vector2F &size = sprite.getSize();
        const Vector2F &origin = sprite.getOrigin();

        element.SetAttribute("texture", getTextureAssetName(sprite.getTextureHandle()).c_str());
        element.SetAttribute("sourceX", source.x);
        element.SetAttribute("sourceY", source.y);
        element.SetAttribute("sourceWidth", source.width);
        element.SetAttribute("sourceHeight", source.height);
        element.SetAttribute("sizeX", size.x);
        element.SetAttribute("sizeY", size.y);
        element.SetAttribute("originX", origin.x);
        element.SetAttribute("originY", origin.y);
    }

    void saveTransformComponent(
        tinyxml2::XMLDocument &document,
        tinyxml2::XMLElement &entityElement,
        const TransformComponent &transform)
    {
        tinyxml2::XMLElement *component = addComponentElement(document, entityElement, "TransformComponent");
        setComponentEnabledAttribute(*component, transform);
        const Vector2F &position = transform.getPosition();
        component->SetAttribute("x", position.x);
        component->SetAttribute("y", position.y);
        component->SetAttribute("rotation", transform.getRotation());
    }

    void saveRectTransformComponent(
        tinyxml2::XMLDocument &document,
        tinyxml2::XMLElement &entityElement,
        const RectTransformComponent &rectTransform)
    {
        tinyxml2::XMLElement *component = addComponentElement(document, entityElement, "RectTransformComponent");
        setComponentEnabledAttribute(*component, rectTransform);

        const Vector2F &position = rectTransform.getAnchoredPosition();
        const Vector2F &size = rectTransform.getSize();
        const Vector2F &pivot = rectTransform.getPivot();

        component->SetAttribute("anchoredX", position.x);
        component->SetAttribute("anchoredY", position.y);
        component->SetAttribute("sizeX", size.x);
        component->SetAttribute("sizeY", size.y);
        component->SetAttribute("pivotX", pivot.x);
        component->SetAttribute("pivotY", pivot.y);
        component->SetAttribute("rotation", rectTransform.getRotation());
    }

    void saveCanvasComponent(
        tinyxml2::XMLDocument &document,
        tinyxml2::XMLElement &entityElement,
        const CanvasComponent &canvas)
    {
        tinyxml2::XMLElement *component = addComponentElement(document, entityElement, "CanvasComponent");
        setComponentEnabledAttribute(*component, canvas);

        const Vector2F &referenceResolution = canvas.getReferenceResolution();
        const RenderColor &canvasColor = canvas.getCanvasColor();

        component->SetAttribute("referenceWidth", referenceResolution.x);
        component->SetAttribute("referenceHeight", referenceResolution.y);
        component->SetAttribute("sortingOrder", canvas.getSortingOrder());
        component->SetAttribute("opacity", canvas.getOpacity());
        component->SetAttribute("scale", canvas.getScale());
        component->SetAttribute("colorR", static_cast<int>(canvasColor.r));
        component->SetAttribute("colorG", static_cast<int>(canvasColor.g));
        component->SetAttribute("colorB", static_cast<int>(canvasColor.b));
        component->SetAttribute("colorA", static_cast<int>(canvasColor.a));
    }

    void saveSpriteComponent(
        tinyxml2::XMLDocument &document,
        tinyxml2::XMLElement &entityElement,
        const SpriteComponent &spriteComponent)
    {
        tinyxml2::XMLElement *component = addComponentElement(document, entityElement, "SpriteComponent");
        setComponentEnabledAttribute(*component, spriteComponent);
        setSpriteAttributes(*component, spriteComponent.getSprite());
    }

    void saveAnimationComponent(
        tinyxml2::XMLDocument &document,
        tinyxml2::XMLElement &entityElement,
        const AnimationComponent &animationComponent)
    {
        tinyxml2::XMLElement *component = addComponentElement(document, entityElement, "AnimationComponent");
        setComponentEnabledAttribute(*component, animationComponent);
        const Animation &animation = animationComponent.getAnimation();
        if (!animationComponent.getAnimationSourcePath().empty())
        {
            component->SetAttribute("source", animationComponent.getAnimationSourcePath().c_str());
        }
        component->SetAttribute("frameDuration", animation.getFrameDuration());
        component->SetAttribute("playing", boolText(animationComponent.isPlaying()));
        component->SetAttribute("looping", boolText(animationComponent.isLooping()));

        for (std::size_t frameIndex = 0; frameIndex < animation.getFrameCount(); ++frameIndex)
        {
            const AnimationRuntimeFrame &runtimeFrame = animation.getRuntimeFrame(frameIndex);
            tinyxml2::XMLElement *frame = document.NewElement("frame");
            frame->SetAttribute("duration", runtimeFrame.duration);
            frame->SetAttribute("sizeX", runtimeFrame.size.x);
            frame->SetAttribute("sizeY", runtimeFrame.size.y);

            for (const AnimationFrameSprite &frameSprite : runtimeFrame.sprites)
            {
                tinyxml2::XMLElement *sprite = document.NewElement("sprite");
                setSpriteAttributes(*sprite, frameSprite.sprite);
                sprite->SetAttribute("localX", frameSprite.localPosition.x);
                sprite->SetAttribute("localY", frameSprite.localPosition.y);
                frame->InsertEndChild(sprite);
            }

            component->InsertEndChild(frame);
        }
    }

    void saveCollisionComponent(
        tinyxml2::XMLDocument &document,
        tinyxml2::XMLElement &entityElement,
        const CollisionComponent &collisionComponent)
    {
        tinyxml2::XMLElement *component = addComponentElement(document, entityElement, "CollisionComponent");
        setComponentEnabledAttribute(*component, collisionComponent);
        const Vector2F &offset = collisionComponent.getOffset();
        component->SetAttribute("name", collisionComponent.getName().c_str());
        component->SetAttribute("width", collisionComponent.getWidth());
        component->SetAttribute("height", collisionComponent.getHeight());
        component->SetAttribute("offsetX", offset.x);
        component->SetAttribute("offsetY", offset.y);
        component->SetAttribute("bodyType", bodyTypeToString(collisionComponent.getBodyType()));
        component->SetAttribute("isSensor", boolText(collisionComponent.isSensor()));
        component->SetAttribute("fixedRotation", boolText(collisionComponent.isFixedRotation()));
    }

    void savePlayerCameraComponent(
        tinyxml2::XMLDocument &document,
        tinyxml2::XMLElement &entityElement,
        const PlayerCameraComponent &cameraComponent)
    {
        tinyxml2::XMLElement *component = addComponentElement(document, entityElement, "PlayerCameraComponent");
        setComponentEnabledAttribute(*component, cameraComponent);
        const Vector2F &offset = cameraComponent.getOffset();
        component->SetAttribute("zoom", cameraComponent.getZoom());
        component->SetAttribute("viewportWidth", cameraComponent.getViewportWidth());
        component->SetAttribute("viewportHeight", cameraComponent.getViewportHeight());
        component->SetAttribute("offsetX", offset.x);
        component->SetAttribute("offsetY", offset.y);
        component->SetAttribute("clampToBounds", boolText(cameraComponent.shouldClampToBounds()));
        component->SetAttribute("minX", cameraComponent.getMinX());
        component->SetAttribute("minY", cameraComponent.getMinY());
        component->SetAttribute("maxX", cameraComponent.getMaxX());
        component->SetAttribute("maxY", cameraComponent.getMaxY());
    }

    void saveScriptValueAttributes(tinyxml2::XMLElement &element, const ScriptValue &value)
    {
        if (value.type == ScriptValueType::Entity
            && value.entityReferenceScope != ScriptEntityReferenceScope::Level)
        {
            element.SetAttribute("scope", scriptEntityReferenceScopeToString(value.entityReferenceScope));
        }

        element.SetAttribute("value", scriptValueToString(value).c_str());
    }

    void saveScriptPropertyAttributes(
        tinyxml2::XMLDocument &document,
        tinyxml2::XMLElement &propertyElement,
        const ScriptProperty &property)
    {
        propertyElement.SetAttribute("name", property.name.c_str());
        propertyElement.SetAttribute("type", scriptPropertyTypeToString(property.type));

        if (property.type == ScriptPropertyType::Array)
        {
            propertyElement.SetAttribute("elementType", scriptValueTypeToString(property.elementType));
            for (const ScriptValue &value : property.arrayValue)
            {
                tinyxml2::XMLElement *itemElement = document.NewElement("item");
                saveScriptValueAttributes(*itemElement, value);
                propertyElement.InsertEndChild(itemElement);
            }
            return;
        }

        if (property.type == ScriptPropertyType::Map)
        {
            propertyElement.SetAttribute("valueType", scriptValueTypeToString(property.mapValueType));
            for (const ScriptMapEntry &entry : property.mapValue)
            {
                tinyxml2::XMLElement *entryElement = document.NewElement("entry");
                entryElement->SetAttribute("key", entry.key.c_str());
                saveScriptValueAttributes(*entryElement, entry.value);
                propertyElement.InsertEndChild(entryElement);
            }
            return;
        }

        if (property.type == ScriptPropertyType::Entity
            && property.entityReferenceScope != ScriptEntityReferenceScope::Level)
        {
            propertyElement.SetAttribute("scope", scriptEntityReferenceScopeToString(property.entityReferenceScope));
        }

        propertyElement.SetAttribute("value", scriptPropertyValueToString(property).c_str());
    }

    void saveScriptComponent(
        tinyxml2::XMLDocument &document,
        tinyxml2::XMLElement &entityElement,
        const ScriptComponent &scriptComponent)
    {
        tinyxml2::XMLElement *component = addComponentElement(document, entityElement, "ScriptComponent");
        setComponentEnabledAttribute(*component, scriptComponent);
        component->SetAttribute("path", scriptComponent.getScriptPath().c_str());

        for (const ScriptProperty &property : scriptComponent.getProperties())
        {
            if (property.name.empty())
            {
                continue;
            }

            tinyxml2::XMLElement *propertyElement = document.NewElement("property");
            saveScriptPropertyAttributes(document, *propertyElement, property);
            component->InsertEndChild(propertyElement);
        }
    }

    void addMarkerComponent(
        tinyxml2::XMLDocument &document,
        tinyxml2::XMLElement &entityElement,
        const char *type,
        const Component &sourceComponent)
    {
        tinyxml2::XMLElement *component = addComponentElement(document, entityElement, type);
        setComponentEnabledAttribute(*component, sourceComponent);
    }

    TextureAsset *getTextureAsset(const tinyxml2::XMLElement &element)
    {
        const char *textureName = element.Attribute("texture");
        if (textureName == nullptr || textureName[0] == '\0')
        {
            return nullptr;
        }

        return AssetManager::getInstance().getTextureAssetByName(textureName);
    }

    Sprite makeSpriteFromAttributes(const tinyxml2::XMLElement &element, TextureAsset &textureAsset)
    {
        RenderRect source;
        source.x = element.FloatAttribute("sourceX", 0.0f);
        source.y = element.FloatAttribute("sourceY", 0.0f);
        source.width = element.FloatAttribute("sourceWidth", 0.0f);
        source.height = element.FloatAttribute("sourceHeight", 0.0f);

        Sprite sprite(textureAsset.getTextureHandle(), source);
        sprite.setSize(Vector2F(
            element.FloatAttribute("sizeX", source.width),
            element.FloatAttribute("sizeY", source.height)));
        sprite.setOrigin(Vector2F(
            element.FloatAttribute("originX", source.width * 0.5f),
            element.FloatAttribute("originY", source.height * 0.5f)));

        return sprite;
    }

    Vector2F calculateAnimationFrameSize(const std::vector<AnimationFrameSprite> &frameSprites)
    {
        if (frameSprites.empty())
        {
            return Vector2F::zero();
        }

        float minX = frameSprites.front().localPosition.x;
        float minY = frameSprites.front().localPosition.y;
        float maxX = frameSprites.front().localPosition.x + frameSprites.front().sprite.getSize().x;
        float maxY = frameSprites.front().localPosition.y + frameSprites.front().sprite.getSize().y;

        for (const AnimationFrameSprite &frameSprite : frameSprites)
        {
            minX = std::min(minX, frameSprite.localPosition.x);
            minY = std::min(minY, frameSprite.localPosition.y);
            maxX = std::max(maxX, frameSprite.localPosition.x + frameSprite.sprite.getSize().x);
            maxY = std::max(maxY, frameSprite.localPosition.y + frameSprite.sprite.getSize().y);
        }

        return Vector2F(maxX - minX, maxY - minY);
    }

    bool loadAnimationFrameSprite(
        const tinyxml2::XMLElement &element,
        AnimationFrameSprite &frameSprite,
        std::string &errorMessage)
    {
        TextureAsset *textureAsset = getTextureAsset(element);
        if (textureAsset == nullptr || textureAsset->getTextureHandle() == nullptr)
        {
            errorMessage = "Level animation frame texture is missing.";
            return false;
        }

        frameSprite = AnimationFrameSprite(
            makeSpriteFromAttributes(element, *textureAsset),
            Vector2F(
                element.FloatAttribute("localX", 0.0f),
                element.FloatAttribute("localY", 0.0f)));
        return true;
    }

    bool addSpriteComponentFromElement(const tinyxml2::XMLElement &component, Entity &entity, std::string &errorMessage)
    {
        TextureAsset *textureAsset = getTextureAsset(component);
        if (textureAsset == nullptr || textureAsset->getTextureHandle() == nullptr)
        {
            errorMessage = "Level sprite texture is missing.";
            return false;
        }

        SpriteComponent &spriteComponent = entity.addComponent<SpriteComponent>(
            makeSpriteFromAttributes(component, *textureAsset));
        applyComponentEnabledAttribute(component, spriteComponent);
        return true;
    }

    bool addAnimationComponentFromElement(const tinyxml2::XMLElement &component, Entity &entity, std::string &errorMessage)
    {
        Animation animation(component.FloatAttribute("frameDuration", 0.1f));
        const char *source = component.Attribute("source");
        const bool hasSource = source != nullptr && source[0] != '\0';
        bool loadedFromSource = false;

        if (hasSource)
        {
            std::string sourceError;
            Animation sourceAnimation;
            if (AnimationLoader::loadFromFile(source, sourceAnimation, sourceError))
            {
                animation = sourceAnimation;
                loadedFromSource = true;
            }
            else if (component.FirstChildElement("frame") == nullptr)
            {
                errorMessage = sourceError;
                return false;
            }
        }

        if (!loadedFromSource)
        {
            for (const tinyxml2::XMLElement *frame = component.FirstChildElement("frame");
                 frame != nullptr;
                 frame = frame->NextSiblingElement("frame"))
            {
                std::vector<AnimationFrameSprite> frameSprites;

                for (const tinyxml2::XMLElement *sprite = frame->FirstChildElement("sprite");
                     sprite != nullptr;
                     sprite = sprite->NextSiblingElement("sprite"))
                {
                    AnimationFrameSprite frameSprite(
                        Sprite(nullptr, RenderRect{}),
                        Vector2F::zero());
                    if (!loadAnimationFrameSprite(*sprite, frameSprite, errorMessage))
                    {
                        return false;
                    }

                    frameSprites.push_back(frameSprite);
                }

                if (frameSprites.empty())
                {
                    AnimationFrameSprite frameSprite(
                        Sprite(nullptr, RenderRect{}),
                        Vector2F::zero());
                    if (!loadAnimationFrameSprite(*frame, frameSprite, errorMessage))
                    {
                        return false;
                    }

                    frameSprites.push_back(frameSprite);
                }

                Vector2F frameSize(
                    frame->FloatAttribute("sizeX", 0.0f),
                    frame->FloatAttribute("sizeY", 0.0f));
                if (frameSize.x <= 0.0f || frameSize.y <= 0.0f)
                {
                    frameSize = calculateAnimationFrameSize(frameSprites);
                }

                animation.addFrame(
                    std::move(frameSprites),
                    frameSize,
                    frame->FloatAttribute("duration", component.FloatAttribute("frameDuration", 0.1f)));
            }
        }

        if (!animation.hasFrames())
        {
            errorMessage = "Level animation component has no frames.";
            return false;
        }

        AnimationComponent &animationComponent = entity.addComponent<AnimationComponent>(animation);
        if (hasSource)
        {
            animationComponent.setAnimationSourcePath(source);
        }
        if (!parseBool(component.Attribute("playing"), true))
        {
            animationComponent.pause();
        }
        animationComponent.setLooping(parseBool(component.Attribute("looping"), true));
        applyComponentEnabledAttribute(component, animationComponent);

        return true;
    }

    ScriptValue loadScriptValueFromElement(const tinyxml2::XMLElement &element, ScriptValueType type)
    {
        ScriptValue value;
        value.type = type;
        if (type == ScriptValueType::Entity)
        {
            value.entityReferenceScope = scriptEntityReferenceScopeFromString(
                element.Attribute("scope") == nullptr ? "Level" : element.Attribute("scope"));
        }
        scriptValueSetValueFromString(value, element.Attribute("value") == nullptr ? "" : element.Attribute("value"));
        return value;
    }

    ScriptProperty loadScriptPropertyFromElement(const tinyxml2::XMLElement &propertyElement)
    {
        ScriptProperty property;
        property.name = propertyElement.Attribute("name") == nullptr ? "" : propertyElement.Attribute("name");
        property.type = scriptPropertyTypeFromString(
            propertyElement.Attribute("type") == nullptr ? "string" : propertyElement.Attribute("type"));
        if (property.type == ScriptPropertyType::Entity)
        {
            property.entityReferenceScope = scriptEntityReferenceScopeFromString(
                propertyElement.Attribute("scope") == nullptr ? "Level" : propertyElement.Attribute("scope"));
        }

        if (property.type == ScriptPropertyType::Array)
        {
            property.elementType = scriptValueTypeFromString(
                propertyElement.Attribute("elementType") == nullptr ? "string" : propertyElement.Attribute("elementType"));
            for (const tinyxml2::XMLElement *itemElement = propertyElement.FirstChildElement("item");
                 itemElement != nullptr;
                 itemElement = itemElement->NextSiblingElement("item"))
            {
                property.arrayValue.push_back(loadScriptValueFromElement(*itemElement, property.elementType));
            }
            return property;
        }

        if (property.type == ScriptPropertyType::Map)
        {
            property.mapValueType = scriptValueTypeFromString(
                propertyElement.Attribute("valueType") == nullptr ? "string" : propertyElement.Attribute("valueType"));
            for (const tinyxml2::XMLElement *entryElement = propertyElement.FirstChildElement("entry");
                 entryElement != nullptr;
                 entryElement = entryElement->NextSiblingElement("entry"))
            {
                ScriptMapEntry entry;
                entry.key = entryElement->Attribute("key") == nullptr ? "" : entryElement->Attribute("key");
                entry.value = loadScriptValueFromElement(*entryElement, property.mapValueType);
                property.mapValue.push_back(std::move(entry));
            }
            return property;
        }

        scriptPropertySetValueFromString(
            property,
            propertyElement.Attribute("value") == nullptr ? "" : propertyElement.Attribute("value"));
        return property;
    }

    std::vector<ScriptProperty> loadScriptPropertiesFromElement(const tinyxml2::XMLElement &component)
    {
        std::vector<ScriptProperty> properties;

        for (const tinyxml2::XMLElement *propertyElement = component.FirstChildElement("property");
             propertyElement != nullptr;
             propertyElement = propertyElement->NextSiblingElement("property"))
        {
            const char *name = propertyElement->Attribute("name");
            if (name == nullptr || name[0] == '\0')
            {
                continue;
            }

            properties.push_back(loadScriptPropertyFromElement(*propertyElement));
        }

        return properties;
    }

    bool addComponentFromElement(const tinyxml2::XMLElement &component, Entity &entity, std::string &errorMessage)
    {
        const char *type = component.Attribute("type");
        if (type == nullptr)
        {
            return true;
        }

        const std::string componentType(type);
        if (componentType == "TransformComponent")
        {
            return true;
        }

        if (componentType == "RectTransformComponent")
        {
            RectTransformComponent *rectTransform = entity.getComponent<RectTransformComponent>();
            if (rectTransform == nullptr)
            {
                rectTransform = &entity.addComponent<RectTransformComponent>(
                    Vector2F(
                        component.FloatAttribute("anchoredX", 0.0f),
                        component.FloatAttribute("anchoredY", 0.0f)),
                    Vector2F(
                        component.FloatAttribute("sizeX", 0.0f),
                        component.FloatAttribute("sizeY", 0.0f)),
                    Vector2F(
                        component.FloatAttribute("pivotX", 0.0f),
                        component.FloatAttribute("pivotY", 0.0f)),
                    component.FloatAttribute("rotation", 0.0f));
            }
            else
            {
                rectTransform->setAnchoredPosition(Vector2F(
                    component.FloatAttribute("anchoredX", 0.0f),
                    component.FloatAttribute("anchoredY", 0.0f)));
                rectTransform->setSize(Vector2F(
                    component.FloatAttribute("sizeX", 0.0f),
                    component.FloatAttribute("sizeY", 0.0f)));
                rectTransform->setPivot(Vector2F(
                    component.FloatAttribute("pivotX", 0.0f),
                    component.FloatAttribute("pivotY", 0.0f)));
                rectTransform->setRotation(component.FloatAttribute("rotation", 0.0f));
            }

            applyComponentEnabledAttribute(component, *rectTransform);
            return true;
        }

        if (componentType == "CanvasComponent")
        {
            CanvasComponent *canvasComponent = entity.getComponent<CanvasComponent>();
            if (canvasComponent == nullptr)
            {
                canvasComponent = &entity.addComponent<CanvasComponent>();
            }

            canvasComponent->setReferenceResolution(Vector2F(
                component.FloatAttribute("referenceWidth", 640.0f),
                component.FloatAttribute("referenceHeight", 320.0f)));
            canvasComponent->setSortingOrder(component.IntAttribute("sortingOrder", 0));
            canvasComponent->setOpacity(component.FloatAttribute("opacity", 1.0f));
            canvasComponent->setScale(component.FloatAttribute("scale", 1.0f));

            RenderColor canvasColor = canvasComponent->getCanvasColor();
            canvasColor.r = readColorAttribute(component, "colorR", canvasColor.r);
            canvasColor.g = readColorAttribute(component, "colorG", canvasColor.g);
            canvasColor.b = readColorAttribute(component, "colorB", canvasColor.b);
            canvasColor.a = readColorAttribute(component, "colorA", canvasColor.a);
            canvasComponent->setCanvasColor(canvasColor);

            if (entity.getComponent<RectTransformComponent>() == nullptr)
            {
                entity.addComponent<RectTransformComponent>(
                    Vector2F::zero(),
                    canvasComponent->getReferenceResolution(),
                    Vector2F(0.5f, 0.5f),
                    0.0f);
            }

            applyComponentEnabledAttribute(component, *canvasComponent);
            return true;
        }

        if (componentType == "SpriteComponent")
        {
            return addSpriteComponentFromElement(component, entity, errorMessage);
        }

        if (componentType == "AnimationComponent")
        {
            return addAnimationComponentFromElement(component, entity, errorMessage);
        }

        if (componentType == "CollisionComponent")
        {
            CollisionComponent &collisionComponent = entity.addComponent<CollisionComponent>(
                component.FloatAttribute("width", 1.0f),
                component.FloatAttribute("height", 1.0f),
                parseBodyType(component.Attribute("bodyType") == nullptr ? "Static" : component.Attribute("bodyType")),
                parseBool(component.Attribute("isSensor"), false),
                component.Attribute("name") == nullptr ? "Collider" : component.Attribute("name"));
            collisionComponent.setOffset(Vector2F(
                component.FloatAttribute("offsetX", 0.0f),
                component.FloatAttribute("offsetY", 0.0f)));
            collisionComponent.setFixedRotation(parseBool(component.Attribute("fixedRotation"), false));
            applyComponentEnabledAttribute(component, collisionComponent);
            return true;
        }

        if (componentType == "PlayerCameraComponent")
        {
            if (entity.getComponent<TransformComponent>() == nullptr)
            {
                entity.addComponent<TransformComponent>(Vector2F(0.0f, 0.0f), 0.0f);
            }

            PlayerCameraComponent &cameraComponent = entity.addComponent<PlayerCameraComponent>(
                component.FloatAttribute("zoom", 1.0f),
                component.FloatAttribute("viewportWidth", 1280.0f),
                component.FloatAttribute("viewportHeight", 720.0f),
                Vector2F(
                    component.FloatAttribute("offsetX", 0.0f),
                    component.FloatAttribute("offsetY", 0.0f)),
                parseBool(component.Attribute("clampToBounds"), false),
                component.FloatAttribute("minX", 0.0f),
                component.FloatAttribute("minY", 0.0f),
                component.FloatAttribute("maxX", 1280.0f),
                component.FloatAttribute("maxY", 720.0f));
            applyComponentEnabledAttribute(component, cameraComponent);
            return true;
        }

        if (componentType == "ScriptComponent")
        {
            const char *path = component.Attribute("path");
            if (path != nullptr && path[0] != '\0')
            {
                ScriptComponent &scriptComponent =
                    entity.addComponent<ScriptComponent>(path, loadScriptPropertiesFromElement(component));
                applyComponentEnabledAttribute(component, scriptComponent);
            }
            return true;
        }

        if (componentType == "PlayerController")
        {
            ScriptComponent &scriptComponent =
                entity.addComponent<ScriptComponent>("Assets/Scripts/player_controller.lua");
            applyComponentEnabledAttribute(component, scriptComponent);
            return true;
        }

        if (componentType == "Bullet")
        {
            return true;
        }

        std::cerr << "Unknown level component: " << componentType << std::endl;
        return true;
    }
}

Level &Level::getCurrentLevel()
{
    static Level currentLevel;
    return currentLevel;
}

Level Level::createEmpty()
{
    return Level();
}

Level &Level::createNewLevel()
{
    Level level = createEmpty();
    level.sourcePath = "Assets/Levels/current.ilevel";
    loadLevel(std::move(level));
    currentLevelPath = "Assets/Levels/current.ilevel";
    return getCurrentLevel();
}

bool Level::saveCurrentLevel(std::string &errorMessage)
{
    return saveCurrentLevel(currentLevelPath, errorMessage);
}

bool Level::saveCurrentLevel(const std::string &path, std::string &errorMessage)
{
    namespace fs = std::filesystem;

    tinyxml2::XMLDocument document;
    document.InsertEndChild(document.NewDeclaration(R"(xml version="1.0" encoding="UTF-8")"));

    tinyxml2::XMLElement *levelElement = document.NewElement("level");
    levelElement->SetAttribute("version", 1);
    document.InsertEndChild(levelElement);

    tinyxml2::XMLElement *entitiesElement = document.NewElement("entities");
    levelElement->InsertEndChild(entitiesElement);

    int savedEntityCount = 0;
    const Level &level = getCurrentLevel();
    for (const Entity &entity : level.getEntities())
    {
        if (entity.isDestroyed())
        {
            continue;
        }

        tinyxml2::XMLElement *entityElement = document.NewElement("entity");
        entityElement->SetAttribute("id", entity.getId());
        entityElement->SetAttribute("guid", entity.getGuid().c_str());
        entityElement->SetAttribute("name", entity.getName().c_str());
        entityElement->SetAttribute("tag", entity.getTag().c_str());
        entityElement->SetAttribute("enabled", entity.isEnabled());
        entityElement->SetAttribute("persistent", entity.isPersistent());
        entityElement->SetAttribute("displayOrder", entity.getDisplayOrder());
        if (const Entity *parent = entity.getParent())
        {
            entityElement->SetAttribute("parentId", parent->getId());
        }
        entitiesElement->InsertEndChild(entityElement);

        ComponentSerializerRegistry::getInstance().saveComponents(document, *entityElement, entity);

        ++savedEntityCount;
    }

    entitiesElement->SetAttribute("count", savedEntityCount);

    const fs::path outputPath(path);
    std::error_code directoryError;
    if (!outputPath.parent_path().empty())
    {
        fs::create_directories(outputPath.parent_path(), directoryError);
        if (directoryError)
        {
            errorMessage = "Failed to create level directory: " + directoryError.message();
            return false;
        }
    }

    const tinyxml2::XMLError result = document.SaveFile(path.c_str());
    if (result != tinyxml2::XML_SUCCESS)
    {
        errorMessage = "Failed to save level: " + std::string(document.ErrorStr());
        return false;
    }

    currentLevelPath = path;
    getCurrentLevel().sourcePath = path;
    errorMessage.clear();
    return true;
}

bool Level::loadFromFile(const std::string &path, Level &level, std::string &errorMessage)
{
    tinyxml2::XMLDocument document;
    if (document.LoadFile(path.c_str()) != tinyxml2::XML_SUCCESS)
    {
        errorMessage = "Failed to load level: " + std::string(document.ErrorStr());
        return false;
    }

    const tinyxml2::XMLElement *levelElement = document.FirstChildElement("level");
    if (levelElement == nullptr)
    {
        errorMessage = "Level file is missing level root.";
        return false;
    }

    const tinyxml2::XMLElement *entitiesElement = levelElement->FirstChildElement("entities");

    Level loadedLevel = createEmpty();
    loadedLevel.sourcePath = path;
    std::unordered_map<int, Entity *> entitiesBySavedId;

    struct PendingParentLink
    {
        Entity *entity = nullptr;
        int parentId = -1;
    };
    std::vector<PendingParentLink> pendingParentLinks;

    int defaultDisplayOrder = 0;
    for (const tinyxml2::XMLElement *entityElement = entitiesElement == nullptr ? nullptr : entitiesElement->FirstChildElement("entity");
         entityElement != nullptr;
         entityElement = entityElement->NextSiblingElement("entity"))
    {
        Entity &entity = loadedLevel.createEntity();
        entity.setComponentStartupDeferred(true);
        const char *savedGuid = entityElement->Attribute("guid");
        if (savedGuid != nullptr && Guid::isValid(savedGuid))
        {
            entity.restoreGuid(savedGuid);
        }
        entity.setName(entityElement->Attribute("name") == nullptr ? "Entity" : entityElement->Attribute("name"));
        entity.setTag(entityElement->Attribute("tag") == nullptr ? "Default" : entityElement->Attribute("tag"));
        entity.setEnabled(entityElement->BoolAttribute("enabled", true));
        entity.setPersistent(entityElement->BoolAttribute("persistent", false));
        entity.setDisplayOrder(entityElement->IntAttribute("displayOrder", defaultDisplayOrder));
        ++defaultDisplayOrder;

        const int savedEntityId = entityElement->IntAttribute("id", -1);
        if (savedEntityId >= 0)
        {
            entitiesBySavedId[savedEntityId] = &entity;
        }

        const int parentId = entityElement->IntAttribute("parentId", -1);
        if (parentId >= 0)
        {
            pendingParentLinks.push_back(PendingParentLink{&entity, parentId});
        }

        ComponentSerializationContext context;
        context.isRootEntity = false;
        context.placement = ComponentSerializationPlacement::None;

        if (!ComponentSerializerRegistry::getInstance().loadComponents(
                *entityElement,
                entity,
                context,
                errorMessage))
        {
            return false;
        }
    }

    for (const PendingParentLink &pendingLink : pendingParentLinks)
    {
        const auto parentIterator = entitiesBySavedId.find(pendingLink.parentId);
        if (pendingLink.entity != nullptr && parentIterator != entitiesBySavedId.end())
        {
            pendingLink.entity->setParent(parentIterator->second, false);
        }
    }

    level = std::move(loadedLevel);
    level.rebuildGuidMap();
    errorMessage.clear();
    return true;
}

bool Level::loadCurrentLevel(std::string &errorMessage)
{
    Level level = createEmpty();
    if (!loadFromFile(currentLevelPath, level, errorMessage))
    {
        return false;
    }

    loadLevel(std::move(level));
    return true;
}

void Level::loadLevel(Level &&level, bool keepPersistentEntities)
{
    Level &currentLevel = getCurrentLevel();
    std::deque<Entity> persistentEntities;
    if (keepPersistentEntities)
    {
        persistentEntities = currentLevel.extractPersistentEntities();
    }

    currentLevel = std::move(level);

    if (!currentLevel.sourcePath.empty())
    {
        currentLevelPath = currentLevel.sourcePath;
    }

    if (!persistentEntities.empty())
    {
        currentLevel.entities.insert(
            currentLevel.entities.end(),
            std::make_move_iterator(persistentEntities.begin()),
            std::make_move_iterator(persistentEntities.end()));
    }

    currentLevel.repairParentChildLinks();
    currentLevel.rebuildGuidMap();
    currentLevel.ensureMainCameraEntity();

    if (EngineState::getInstance().isPlaying())
    {
        startPendingComponents();
    }
}

void Level::startPendingComponents()
{
    if (!EngineState::getInstance().isPlaying())
    {
        return;
    }

    Level &currentLevel = getCurrentLevel();
    InputSystem &inputSystem = InputSystem::getInstance();
    std::size_t index = 0;
    while (index < currentLevel.entities.size())
    {
        Entity &entity = currentLevel.entities[index];
        entity.startDeferredComponents();
        entity.addInputListeners(inputSystem);
        ++index;
    }
}

const std::string &Level::getCurrentLevelPath()
{
    return currentLevelPath;
}

void Level::setCurrentLevelPath(const std::string &path)
{
    currentLevelPath = path;
    getCurrentLevel().sourcePath = path;
}

Entity &Level::createEntity()
{
    entities.emplace_back();
    Entity &entity = entities.back();
    entityGuidMap[entity.getGuid()] = &entity;
    return entity;
}

Entity &Level::addEntity(Entity entity)
{
    entities.push_back(std::move(entity));
    Entity &addedEntity = entities.back();
    entityGuidMap[addedEntity.getGuid()] = &addedEntity;
    return addedEntity;
}

Entity *Level::duplicateEntity(const Entity &source, bool duplicateChildren)
{
    GuidRemap guidRemap;
    std::vector<Entity *> createdEntities;

    Entity *copy = duplicateEntityRecursive(
        *this,
        source,
        nullptr,
        true,
        duplicateChildren,
        guidRemap,
        createdEntities);
    if (copy == nullptr)
    {
        return nullptr;
    }

    remapDuplicatedScriptReferences(createdEntities, guidRemap);

    if (EngineState::getInstance().isPlaying())
    {
        InputSystem &inputSystem = InputSystem::getInstance();
        for (Entity *entity : createdEntities)
        {
            if (entity == nullptr || entity->isDestroyed())
            {
                continue;
            }

            entity->startDeferredComponents();
            entity->addInputListeners(inputSystem);
        }
    }

    rebuildGuidMap();
    repairParentChildLinks();
    return copy;
}

bool Level::removeEntity(std::size_t index)
{
    if (index >= entities.size())
    {
        return false;
    }

    entities[index].destroy();
    entities.erase(entities.begin() + static_cast<std::deque<Entity>::difference_type>(index));
    rebuildGuidMap();
    return true;
}

bool Level::destroyEntity(Entity &entity)
{
    return destroyEntity(&entity);
}

bool Level::destroyEntity(Entity *entity)
{
    if (entity == nullptr)
    {
        return false;
    }

    for (Entity &storedEntity : entities)
    {
        if (&storedEntity == entity)
        {
            storedEntity.destroy();
            return true;
        }
    }

    return false;
}

bool Level::destroyEntityById(int id)
{
    for (Entity &entity : entities)
    {
        if (entity.getId() == id)
        {
            entity.destroy();
            return true;
        }
    }

    return false;
}

bool Level::destroyEntityHierarchy(Entity &entity)
{
    return destroyEntityHierarchy(&entity);
}

bool Level::destroyEntityHierarchy(Entity *entity)
{
    if (entity == nullptr)
    {
        return false;
    }

    for (Entity &storedEntity : entities)
    {
        if (&storedEntity == entity)
        {
            storedEntity.destroy(true);
            return true;
        }
    }

    return false;
}

bool Level::destroyEntityByIdHierarchy(int id)
{
    for (Entity &entity : entities)
    {
        if (entity.getId() == id)
        {
            entity.destroy(true);
            return true;
        }
    }

    return false;
}

void Level::cleanupDestroyedEntities()
{
    entities.erase(
        std::remove_if(
            entities.begin(),
            entities.end(),
            [](const Entity &entity)
            {
                return entity.isDestroyed();
            }),
        entities.end());

    repairParentChildLinks();
    rebuildGuidMap();
}

void Level::clearEntities()
{
    for (Entity &entity : entities)
    {
        entity.destroy();
    }

    entities.clear();
    entityGuidMap.clear();
}

Entity *Level::getEntity(std::size_t index)
{
    if (index >= entities.size())
    {
        return nullptr;
    }

    return &entities[index];
}

const Entity *Level::getEntity(std::size_t index) const
{
    if (index >= entities.size())
    {
        return nullptr;
    }

    return &entities[index];
}

Entity *Level::findEntityById(int id)
{
    for (Entity &entity : entities)
    {
        if (entity.getId() == id && !entity.isDestroyed())
        {
            return &entity;
        }
    }

    return nullptr;
}

const Entity *Level::findEntityById(int id) const
{
    for (const Entity &entity : entities)
    {
        if (entity.getId() == id && !entity.isDestroyed())
        {
            return &entity;
        }
    }

    return nullptr;
}

std::deque<Entity> &Level::getEntities()
{
    return entities;
}

const std::deque<Entity> &Level::getEntities() const
{
    return entities;
}

const std::string &Level::getSourcePath() const
{
    return sourcePath;
}

std::string Level::getSourceFileName() const
{
    return std::filesystem::path(sourcePath).stem().string();
}

void Level::update(float deltaTime)
{
    for (Entity &entity : entities)
    {
        if (!entity.isDestroyed())
        {
            entity.applyDeferredComponentEnableChange();
        }
    }

    for (Entity &entity : entities)
    {
        if (entity.isDestroyed() || !entity.isEnabled())
        {
            continue;
        }

        entity.update(deltaTime);
    }

    for (Entity &entity : entities)
    {
        if (entity.isDestroyed() || !entity.isEnabled())
        {
            continue;
        }

        CollisionComponent *collisionComponent = entity.getComponent<CollisionComponent>();
        if (collisionComponent != nullptr)
        {
            collisionComponent->syncBodyToTransform();
        }
    }

    cleanupDestroyedEntities();
}

std::deque<Entity> Level::extractPersistentEntities()
{
    std::deque<Entity> persistentEntities;
    InputSystem &inputSystem = InputSystem::getInstance();

    for (Entity &entity : entities)
    {
        if (entity.isPersistent())
        {
            Entity *parent = entity.getParent();
            if (parent != nullptr && !parent->isPersistent())
            {
                entity.clearParent(false);
            }

            entity.removeInputListeners(inputSystem);
            persistentEntities.push_back(std::move(entity));
        }
    }
    return persistentEntities;
}

void Level::ensureMainCameraEntity()
{
    PlayerCameraComponent defaultCameraComponent;
    const Vector2F defaultCameraPosition(
        defaultCameraComponent.getViewportWidth() * 0.5f,
        defaultCameraComponent.getViewportHeight() * 0.5f);

    for (Entity &entity : entities)
    {
        if (entity.isDestroyed() || entity.getTag() != "MainCamera")
        {
            continue;
        }

        if (entity.getComponent<TransformComponent>() == nullptr)
        {
            entity.addComponent<TransformComponent>(defaultCameraPosition, 0.0f);
        }

        if (entity.getComponent<PlayerCameraComponent>() == nullptr)
        {
            entity.addComponent<PlayerCameraComponent>();
        }

        return;
    }

    Entity &cameraEntity = createEntity();
    cameraEntity.setName("MainCamera");
    cameraEntity.setTag("MainCamera");
    cameraEntity.addComponent<TransformComponent>(defaultCameraPosition, 0.0f);
    cameraEntity.addComponent<PlayerCameraComponent>();
}

void Level::rebuildGuidMap()
{
    entityGuidMap.clear();

    for (Entity &entity : entities)
    {
        if (!entity.isDestroyed() && !entity.getGuid().empty())
        {
            entityGuidMap[entity.getGuid()] = &entity;
        }
    }
}

void Level::repairParentChildLinks()
{
    std::unordered_map<std::string, Entity *> entitiesByGuid;
    for (Entity &entity : entities)
    {
        if (!entity.isDestroyed() && !entity.getGuid().empty())
        {
            entitiesByGuid[entity.getGuid()] = &entity;
        }
    }

    for (Entity &entity : entities)
    {
        entity.children.clear();
    }

    for (Entity &entity : entities)
    {
        if (entity.isDestroyed())
        {
            entity.parentEntity = nullptr;
            entity.parentGuid.clear();
            continue;
        }

        Entity *parent = nullptr;
        if (!entity.parentGuid.empty())
        {
            const auto parentIterator = entitiesByGuid.find(entity.parentGuid);
            if (parentIterator != entitiesByGuid.end())
            {
                parent = parentIterator->second;
            }
        }
        else if (entity.parentEntity != nullptr && !entity.parentEntity->getGuid().empty())
        {
            const auto parentIterator = entitiesByGuid.find(entity.parentEntity->getGuid());
            if (parentIterator != entitiesByGuid.end())
            {
                parent = parentIterator->second;
                entity.parentGuid = parent->getGuid();
            }
        }

        if (parent == nullptr || parent == &entity || parent->isChildOf(entity))
        {
            entity.parentEntity = nullptr;
            entity.parentGuid.clear();
            entity.syncTransformParent();
            continue;
        }

        entity.parentEntity = parent;
        entity.parentGuid = parent->getGuid();

        if (std::find(parent->children.begin(), parent->children.end(), &entity) == parent->children.end())
        {
            parent->children.push_back(&entity);
        }

        entity.syncTransformParent();
    }
}

Entity *Level::getEntityByGuid(const std::string &guid)
{
    auto it = entityGuidMap.find(guid);

    if (it != entityGuidMap.end() && it->second != nullptr && !it->second->isDestroyed())
    {
        return it->second;
    }
    return nullptr;
}

const Entity *Level::getEntityByGuid(const std::string &guid) const
{
    auto it = entityGuidMap.find(guid);
    if (it != entityGuidMap.end() && it->second != nullptr && !it->second->isDestroyed())
    {
        return it->second;
    }
    return nullptr;
}
