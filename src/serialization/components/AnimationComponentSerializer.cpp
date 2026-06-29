#include "serialization/components/AnimationComponentSerializer.h"

#include "Entity.h"
#include "components/AnimationComponent.h"
#include "misc/Animation.h"
#include "misc/AnimationLoader.h"
#include "misc/TextureAsset.h"
#include "serialization/ComponentSerializationHelpers.h"

#include "tinyxml2.h"

#include <algorithm>
#include <sstream>
#include <utility>
#include <vector>

namespace {
Vector2F calculateFrameSize(const std::vector<AnimationFrameSprite>& frameSprites) {
    if (frameSprites.empty()) {
        return Vector2F::zero();
    }

    float minX = frameSprites.front().localPosition.x;
    float minY = frameSprites.front().localPosition.y;
    float maxX = frameSprites.front().localPosition.x + frameSprites.front().sprite.getSize().x;
    float maxY = frameSprites.front().localPosition.y + frameSprites.front().sprite.getSize().y;

    for (const AnimationFrameSprite& frameSprite : frameSprites) {
        minX = std::min(minX, frameSprite.localPosition.x);
        minY = std::min(minY, frameSprite.localPosition.y);
        maxX = std::max(maxX, frameSprite.localPosition.x + frameSprite.sprite.getSize().x);
        maxY = std::max(maxY, frameSprite.localPosition.y + frameSprite.sprite.getSize().y);
    }

    return Vector2F(maxX - minX, maxY - minY);
}

bool loadFrameSprite(
    const tinyxml2::XMLElement& element,
    AnimationFrameSprite& frameSprite,
    std::string& errorMessage
) {
    TextureAsset* textureAsset = ComponentSerializationHelpers::getTextureAsset(element);
    if (textureAsset == nullptr || textureAsset->getTextureHandle() == nullptr) {
        std::ostringstream stream;
        stream << "Prefab animation frame texture is missing: "
               << (element.Attribute("texture") == nullptr ? "<none>" : element.Attribute("texture"));
        errorMessage = stream.str();
        return false;
    }

    frameSprite = AnimationFrameSprite(
        ComponentSerializationHelpers::makeSpriteFromAttributes(element, *textureAsset),
        Vector2F(
            element.FloatAttribute("localX", 0.0f),
            element.FloatAttribute("localY", 0.0f)
        )
    );
    return true;
}
}

const char* AnimationComponentSerializer::getTypeName() const {
    return "AnimationComponent";
}

bool AnimationComponentSerializer::hasComponent(const Entity& entity) const {
    return entity.getComponent<AnimationComponent>() != nullptr;
}

void AnimationComponentSerializer::save(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const Entity& entity
) const {
    const AnimationComponent* animationComponent = entity.getComponent<AnimationComponent>();
    if (animationComponent == nullptr) {
        return;
    }

    tinyxml2::XMLElement* component =
        ComponentSerializationHelpers::addComponentElement(document, entityElement, getTypeName());
    ComponentSerializationHelpers::setComponentEnabledAttribute(*component, *animationComponent);
    const Animation& animation = animationComponent->getAnimation();
    if (!animationComponent->getAnimationSourcePath().empty()) {
        component->SetAttribute("source", animationComponent->getAnimationSourcePath().c_str());
    }
    component->SetAttribute("frameDuration", animation.getFrameDuration());
    component->SetAttribute("playing", ComponentSerializationHelpers::boolText(animationComponent->isPlaying()));
    component->SetAttribute("looping", ComponentSerializationHelpers::boolText(animationComponent->isLooping()));

    for (std::size_t frameIndex = 0; frameIndex < animation.getFrameCount(); ++frameIndex) {
        const AnimationRuntimeFrame& runtimeFrame = animation.getRuntimeFrame(frameIndex);
        tinyxml2::XMLElement* frame = document.NewElement("frame");
        frame->SetAttribute("duration", runtimeFrame.duration);
        frame->SetAttribute("sizeX", runtimeFrame.size.x);
        frame->SetAttribute("sizeY", runtimeFrame.size.y);

        for (const AnimationFrameSprite& frameSprite : runtimeFrame.sprites) {
            tinyxml2::XMLElement* sprite = document.NewElement("sprite");
            ComponentSerializationHelpers::setSpriteAttributes(*sprite, frameSprite.sprite);
            sprite->SetAttribute("localX", frameSprite.localPosition.x);
            sprite->SetAttribute("localY", frameSprite.localPosition.y);
            frame->InsertEndChild(sprite);
        }

        component->InsertEndChild(frame);
    }
}

bool AnimationComponentSerializer::load(
    const tinyxml2::XMLElement& componentElement,
    Entity& entity,
    const ComponentSerializationContext& context,
    std::string& errorMessage
) const {
    (void)context;

    Animation animation(componentElement.FloatAttribute("frameDuration", 0.1f));
    const char* source = componentElement.Attribute("source");
    const bool hasSource = source != nullptr && source[0] != '\0';
    bool loadedFromSource = false;

    if (hasSource) {
        std::string sourceError;
        Animation sourceAnimation;
        if (AnimationLoader::loadFromFile(source, sourceAnimation, sourceError)) {
            animation = sourceAnimation;
            loadedFromSource = true;
        } else if (componentElement.FirstChildElement("frame") == nullptr) {
            errorMessage = sourceError;
            return false;
        }
    }

    if (!loadedFromSource) {
        for (const tinyxml2::XMLElement* frame = componentElement.FirstChildElement("frame");
             frame != nullptr;
             frame = frame->NextSiblingElement("frame")) {
            std::vector<AnimationFrameSprite> frameSprites;

            for (const tinyxml2::XMLElement* sprite = frame->FirstChildElement("sprite");
                 sprite != nullptr;
                 sprite = sprite->NextSiblingElement("sprite")) {
                AnimationFrameSprite frameSprite(
                    Sprite(nullptr, RenderRect{}),
                    Vector2F::zero()
                );
                if (!loadFrameSprite(*sprite, frameSprite, errorMessage)) {
                    return false;
                }

                frameSprites.push_back(frameSprite);
            }

            if (frameSprites.empty()) {
                AnimationFrameSprite frameSprite(
                    Sprite(nullptr, RenderRect{}),
                    Vector2F::zero()
                );
                if (!loadFrameSprite(*frame, frameSprite, errorMessage)) {
                    return false;
                }

                frameSprites.push_back(frameSprite);
            }

            Vector2F frameSize(
                frame->FloatAttribute("sizeX", 0.0f),
                frame->FloatAttribute("sizeY", 0.0f)
            );
            if (frameSize.x <= 0.0f || frameSize.y <= 0.0f) {
                frameSize = calculateFrameSize(frameSprites);
            }

            animation.addFrame(
                std::move(frameSprites),
                frameSize,
                frame->FloatAttribute("duration", componentElement.FloatAttribute("frameDuration", 0.1f))
            );
        }
    }

    if (!animation.hasFrames()) {
        errorMessage = "Prefab animation component has no frames.";
        return false;
    }

    AnimationComponent& animationComponent = entity.addComponent<AnimationComponent>(animation);
    if (hasSource) {
        animationComponent.setAnimationSourcePath(source);
    }
    if (!ComponentSerializationHelpers::parseBool(componentElement.Attribute("playing"), true)) {
        animationComponent.pause();
    }
    animationComponent.setLooping(ComponentSerializationHelpers::parseBool(
        componentElement.Attribute("looping"),
        true
    ));

    ComponentSerializationHelpers::applyComponentEnabledAttribute(componentElement, animationComponent);
    return true;
}
