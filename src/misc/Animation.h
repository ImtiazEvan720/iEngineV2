#pragma once

#include "misc/Sprite.h"

#include <cstddef>
#include <vector>

struct AnimationFrameSprite {
    AnimationFrameSprite(const Sprite& sprite, const Vector2F& localPosition);

    Sprite sprite;
    Vector2F localPosition;
};

struct AnimationRuntimeFrame {
    AnimationRuntimeFrame(
        std::vector<AnimationFrameSprite> sprites,
        const Vector2F& size,
        float duration
    );

    std::vector<AnimationFrameSprite> sprites;
    Vector2F size;
    float duration;
};

class Animation {
public:
    Animation() = default;
    explicit Animation(float frameDuration);

    void addFrame(const Sprite& sprite);
    void addFrame(
        std::vector<AnimationFrameSprite> sprites,
        const Vector2F& size,
        float frameDuration
    );
    void clearFrames();

    const Sprite& getFrame(std::size_t index) const;
    const AnimationRuntimeFrame& getRuntimeFrame(std::size_t index) const;
    const std::vector<AnimationFrameSprite>& getFrameSprites(std::size_t index) const;
    bool hasFrames() const;
    std::size_t getFrameCount() const;

    float getFrameDuration() const;
    float getFrameDuration(std::size_t index) const;
    const Vector2F& getFrameSize(std::size_t index) const;
    void setFrameDuration(float frameDuration);

private:
    std::vector<AnimationRuntimeFrame> frames;
    float frameDuration = 0.1f;
};
