#include "components/AnimationComponent.h"

#include "Entity.h"
#include "components/SpriteComponent.h"

#include <stdexcept>

AnimationComponent::AnimationComponent(const Animation& animation)
    : animation(animation) {}

Animation& AnimationComponent::getAnimation() {
    return animation;
}

const Animation& AnimationComponent::getAnimation() const {
    return animation;
}

void AnimationComponent::setAnimation(const Animation& animation) {
    this->animation = animation;
    reset();
    playing = true;
}

void AnimationComponent::play() {
    if (finished) {
        reset();
    }

    playing = true;
}

void AnimationComponent::pause() {
    playing = false;
}

void AnimationComponent::reset() {
    currentFrameIndex = 0;
    elapsedTime = 0.0f;
    finished = false;
}

bool AnimationComponent::isPlaying() const {
    return playing;
}

bool AnimationComponent::isFinished() const {
    return finished;
}

std::size_t AnimationComponent::getCurrentFrameIndex() const {
    return currentFrameIndex;
}

void AnimationComponent::setLooping(bool looping) {
    this->looping = looping;
}

const Sprite& AnimationComponent::getCurrentFrame() const {
    if (!animation.hasFrames()) {
        throw std::runtime_error("AnimationComponent has no frames.");
    }

    return animation.getFrame(currentFrameIndex);
}

void AnimationComponent::onUpdate(float deltaTime) {
    if (!animation.hasFrames() || !this->isEnabled()) {
        return;
    }

    if (currentFrameIndex >= animation.getFrameCount()) {
        currentFrameIndex = 0;
    }

    if (playing && !finished && animation.getFrameDuration() > 0.0f) {
        elapsedTime += deltaTime;

        while (elapsedTime >= animation.getFrameDuration()) {
            elapsedTime -= animation.getFrameDuration();

            if (currentFrameIndex + 1 < animation.getFrameCount()) {
                ++currentFrameIndex;
            } else if (looping) {
                currentFrameIndex = 0;
            } else {
                finished = true;
                playing = false;
                break;
            }
        }
    }

    Entity* entity = getEntity();
    if (entity == nullptr) {
        return;
    }

    SpriteComponent* spriteComponent = entity->getComponent<SpriteComponent>();
    if (spriteComponent == nullptr) {
        return;
    }

    spriteComponent->setSprite(getCurrentFrame());
}

std::unique_ptr<Component> AnimationComponent::clone() const {
    auto copy = std::make_unique<AnimationComponent>(animation);
    copy->setLooping(looping);

    if (!playing) {
        copy->pause();
    }

    copy->setEnabled(isEnabled());
    return copy;
}
