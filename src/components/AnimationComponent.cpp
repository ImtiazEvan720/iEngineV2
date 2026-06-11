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
    if (!animation.hasFrames() || getEntity() == nullptr) {
        return;
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

    SpriteComponent* spriteComponent = getEntity()->getComponent<SpriteComponent>();
    if (spriteComponent == nullptr) {
        return;
    }

    spriteComponent->setSprite(getCurrentFrame());
}
