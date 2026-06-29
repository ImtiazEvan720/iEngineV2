#pragma once

#include "misc/Animation.h"

#include <string>

class AnimationLoader {
public:
    static bool loadFromFile(
        const std::string& path,
        Animation& animation,
        std::string& errorMessage
    );
};
