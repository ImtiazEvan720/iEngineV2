#pragma once

#include "misc/Sprite.h"

#include <cstddef>
#include <vector>

class Animation {
public:
    Animation() = default;
    explicit Animation(float frameDuration);

    void addFrame(const Sprite& sprite);
    void clearFrames();

    const Sprite& getFrame(std::size_t index) const;
    bool hasFrames() const;
    std::size_t getFrameCount() const;

    float getFrameDuration() const;
    void setFrameDuration(float frameDuration);

private:
    std::vector<Sprite> frames;
    float frameDuration = 0.1f;
};
