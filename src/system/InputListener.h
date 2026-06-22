#pragma once

#include "system/InputTypes.h"

class InputListener {
public:
    virtual ~InputListener() = default;

    virtual void onKeyPressed(InputKey key);
    virtual void onKeyReleased(InputKey key);
    virtual void onMousePressed(InputMouseButton button, int x, int y);
    virtual void onMouseReleased(InputMouseButton button, int x, int y);
};
