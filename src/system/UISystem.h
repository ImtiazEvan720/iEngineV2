#pragma once

#include "math/Vector2F.h"

class Entity;
class IWindowBackend;
class RawInputSystem;
class RectTransformComponent;

class UISystem {
public:
    static UISystem& getInstance();

    UISystem(const UISystem& other) = delete;
    UISystem& operator=(const UISystem& other) = delete;
    UISystem(UISystem&& other) = delete;
    UISystem& operator=(UISystem&& other) = delete;

    bool processInput(const RawInputSystem& rawInputSystem, IWindowBackend* windowBackend);

private:
    UISystem() = default;

    Entity* findTopmostButtonAt(const Vector2F& screenPosition);
    Entity* findTopmostEditTextAt(const Vector2F& screenPosition);
    void setFocusedEditText(Entity* entity, IWindowBackend* windowBackend);
    void updateFocusedEditText(const RawInputSystem& rawInputSystem, IWindowBackend* windowBackend);
    bool isPointInsideRectTransform(
        const RectTransformComponent& rectTransform,
        const Vector2F& screenPosition,
        float scale
    ) const;

    int pressedEntityId = -1;
    int activeTouchId = -1;
    int focusedEditTextEntityId = -1;
};
