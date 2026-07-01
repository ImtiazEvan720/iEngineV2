#pragma once

#include "components/Component.h"
#include "misc/Animation.h"

#include <string>
#include <vector>

class AnimationComponent : public Component {
public:
    explicit AnimationComponent(const Animation& animation);

    Animation& getAnimation();
    const Animation& getAnimation() const;

    void setAnimation(const Animation& animation);
    void setAnimationSourcePath(const std::string& path);
    const std::string& getAnimationSourcePath() const;

    void play();
    void pause();
    void reset();
    bool isPlaying() const;
    bool isLooping() const;
    bool isFinished() const;
    std::size_t getCurrentFrameIndex() const;
    void setLooping(bool looping);
    void notifyAnimationFinished();

    const Sprite& getCurrentFrame() const;
    const AnimationRuntimeFrame& getCurrentRuntimeFrame() const;
    const std::vector<AnimationFrameSprite>& getCurrentFrameSprites() const;
    void onUpdate(float deltaTime) override;
    std::unique_ptr<Component> clone() const override;

private:
    Animation animation;
    std::string animationSourcePath;
    std::size_t currentFrameIndex = 0;
    float elapsedTime = 0.0f;
    bool playing = true;
    bool looping = true;
    bool finished = false;
};
