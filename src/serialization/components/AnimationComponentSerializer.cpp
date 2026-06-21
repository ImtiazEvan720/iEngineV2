#include "serialization/components/AnimationComponentSerializer.h"

#include "Entity.h"
#include "components/AnimationComponent.h"
#include "misc/Animation.h"
#include "misc/TextureAsset.h"
#include "serialization/ComponentSerializationHelpers.h"

#include "tinyxml2.h"

#include <sstream>

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
    const Animation& animation = animationComponent->getAnimation();
    component->SetAttribute("frameDuration", animation.getFrameDuration());
    component->SetAttribute("playing", ComponentSerializationHelpers::boolText(animationComponent->isPlaying()));

    for (std::size_t frameIndex = 0; frameIndex < animation.getFrameCount(); ++frameIndex) {
        tinyxml2::XMLElement* frame = document.NewElement("frame");
        ComponentSerializationHelpers::setSpriteAttributes(*frame, animation.getFrame(frameIndex));
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

    for (const tinyxml2::XMLElement* frame = componentElement.FirstChildElement("frame");
         frame != nullptr;
         frame = frame->NextSiblingElement("frame")) {
        TextureAsset* textureAsset = ComponentSerializationHelpers::getTextureAsset(*frame);
        if (textureAsset == nullptr || textureAsset->getTextureHandle() == nullptr) {
            std::ostringstream stream;
            stream << "Prefab animation frame texture is missing: "
                   << (frame->Attribute("texture") == nullptr ? "<none>" : frame->Attribute("texture"));
            errorMessage = stream.str();
            return false;
        }

        animation.addFrame(ComponentSerializationHelpers::makeSpriteFromAttributes(*frame, *textureAsset));
    }

    if (!animation.hasFrames()) {
        errorMessage = "Prefab animation component has no frames.";
        return false;
    }

    AnimationComponent& animationComponent = entity.addComponent<AnimationComponent>(animation);
    if (!ComponentSerializationHelpers::parseBool(componentElement.Attribute("playing"), true)) {
        animationComponent.pause();
    }

    return true;
}
