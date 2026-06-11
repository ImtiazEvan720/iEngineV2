#include "misc/Animation.h"

#include <stdexcept>

Animation::Animation(float frameDuration)
    : frameDuration(frameDuration) {}

void Animation::addFrame(const Sprite& frame) {
    frames.push_back(frame);
}

void Animation::clearFrames() {
    frames.clear();
}

const Sprite& Animation::getFrame(std::size_t index) const {
    if (frames.empty()) {
        throw std::runtime_error("Animation has no frames.");
    }

    if (index >= frames.size()) {
        throw std::out_of_range("Animation frame index is out of range.");
    }

    return frames[index];
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

void Animation::setFrameDuration(float frameDuration) {
    this->frameDuration = frameDuration;
}
