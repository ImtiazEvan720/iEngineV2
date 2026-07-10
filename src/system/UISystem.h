#pragma once

#include "math/Vector2F.h"

class Entity;
class RawInputSystem;
class RectTransformComponent;

class UISystem {
public:
    static UISystem& getInstance();

    UISystem(const UISystem& other) = delete;
    UISystem& operator=(const UISystem& other) = delete;
    UISystem(UISystem&& other) = delete;
    UISystem& operator=(UISystem&& other) = delete;

    bool processInput(const RawInputSystem& rawInputSystem);

private:
    UISystem() = default;

    Entity* findTopmostButtonAt(const Vector2F& screenPosition);
    bool isPointInsideRectTransform(
        const RectTransformComponent& rectTransform,
        const Vector2F& screenPosition,
        float scale
    ) const;

    int pressedEntityId = -1;
    int activeTouchId = -1;
};
