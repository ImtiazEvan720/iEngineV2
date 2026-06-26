#pragma once

#include "components/Component.h"
#include "misc/Animation.h"

class AnimationComponent : public Component {
public:
    explicit AnimationComponent(const Animation& animation);

    Animation& getAnimation();
    const Animation& getAnimation() const;

    void setAnimation(const Animation& animation);

    void play();
    void pause();
    void reset();
    bool isPlaying() const;
    bool isFinished() const;
    std::size_t getCurrentFrameIndex() const;
    void setLooping(bool looping);

    const Sprite& getCurrentFrame() const;
    void onUpdate(float deltaTime) override;
    std::unique_ptr<Component> clone() const override;

private:
    Animation animation;
    std::size_t currentFrameIndex = 0;
    float elapsedTime = 0.0f;
    bool playing = true;
    bool looping = true;
    bool finished = false;
};
