#include "misc/Animation.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace {
float sanitizeDuration(float frameDuration, float fallback) {
    if (frameDuration > 0.0f) {
        return frameDuration;
    }

    if (fallback > 0.0f) {
        return fallback;
    }

    return 0.1f;
}
}

AnimationFrameSprite::AnimationFrameSprite(const Sprite& sprite, const Vector2F& localPosition)
    : sprite(sprite),
      localPosition(localPosition) {}

AnimationRuntimeFrame::AnimationRuntimeFrame(
    std::vector<AnimationFrameSprite> sprites,
    const Vector2F& size,
    float duration
)
    : sprites(std::move(sprites)),
      size(size),
      duration(duration) {}

Animation::Animation(float frameDuration)
    : frameDuration(sanitizeDuration(frameDuration, 0.1f)) {}

void Animation::addFrame(const Sprite& frame) {
    std::vector<AnimationFrameSprite> frameSprites;
    frameSprites.emplace_back(frame, Vector2F::zero());
    addFrame(std::move(frameSprites), frame.getSize(), frameDuration);
}

void Animation::addFrame(
    std::vector<AnimationFrameSprite> sprites,
    const Vector2F& size,
    float frameDuration
) {
    if (sprites.empty()) {
        throw std::invalid_argument("Animation frame must contain at least one sprite.");
    }

    const float sanitizedDuration = sanitizeDuration(frameDuration, this->frameDuration);
    if (frames.empty()) {
        this->frameDuration = sanitizedDuration;
    }

    frames.emplace_back(std::move(sprites), size, sanitizedDuration);
}

void Animation::clearFrames() {
    frames.clear();
}

const Sprite& Animation::getFrame(std::size_t index) const {
    const std::vector<AnimationFrameSprite>& frameSprites = getFrameSprites(index);
    if (frameSprites.empty()) {
        throw std::runtime_error("Animation frame has no sprites.");
    }

    return frameSprites.front().sprite;
}

const AnimationRuntimeFrame& Animation::getRuntimeFrame(std::size_t index) const {
    if (frames.empty()) {
        throw std::runtime_error("Animation has no frames.");
    }

    if (index >= frames.size()) {
        throw std::out_of_range("Animation frame index is out of range.");
    }

    return frames[index];
}

const std::vector<AnimationFrameSprite>& Animation::getFrameSprites(std::size_t index) const {
    return getRuntimeFrame(index).sprites;
}

bool Animation::hasFrames() const {
    return !frames.empty();
}

std::size_t Animation::getFrameCount() const {
    return frames.size();
}

float Animation::getFrameDuration() const {
    return frameDuration;
}

float Animation::getFrameDuration(std::size_t index) const {
    return getRuntimeFrame(index).duration;
}

const Vector2F& Animation::getFrameSize(std::size_t index) const {
    return getRuntimeFrame(index).size;
}

void Animation::setFrameDuration(float frameDuration) {
    this->frameDuration = sanitizeDuration(frameDuration, this->frameDuration);

    for (AnimationRuntimeFrame& frame : frames) {
        frame.duration = this->frameDuration;
    }
}
