#include "system/UISystem.h"

#include "Entity.h"
#include "components/CanvasComponent.h"
#include "components/RectTransformComponent.h"
#include "components/UIButtonComponent.h"
#include "math/Math2D.h"
#include "misc/Level.h"
#include "system/RawInputSystem.h"
#include "system/ScriptSystem.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {
struct PointerState {
    Vector2F position = Vector2F::zero();
    bool down = false;
    bool pressedThisFrame = false;
    bool releasedThisFrame = false;
};

const CanvasComponent* getParentCanvas(const Entity& entity) {
    for (const Entity* parent = entity.getParent();
         parent != nullptr;
         parent = parent->getParent()) {
        const CanvasComponent* canvas = parent->getComponent<CanvasComponent>();
        if (canvas != nullptr) {
            return canvas;
        }
    }

    return nullptr;
}

bool parentCanvasAllowsInput(const Entity& entity) {
    for (const Entity* parent = entity.getParent();
         parent != nullptr;
         parent = parent->getParent()) {
        if (!parent->isEnabled()) {
            return false;
        }

        const CanvasComponent* canvas = parent->getComponent<CanvasComponent>();
        if (canvas != nullptr && !canvas->isEnabled()) {
            return false;
        }
    }

    return true;
}

int getUiSortingOrder(const Entity& entity) {
    const CanvasComponent* canvas = entity.getComponent<CanvasComponent>();
    if (canvas != nullptr) {
        return canvas->getSortingOrder();
    }

    const CanvasComponent* parentCanvas = getParentCanvas(entity);
    return parentCanvas == nullptr ? 0 : parentCanvas->getSortingOrder();
}

float getUiScale(const Entity& entity) {
    const CanvasComponent* canvas = entity.getComponent<CanvasComponent>();
    if (canvas != nullptr) {
        return std::max(0.001f, canvas->getScale());
    }

    const CanvasComponent* parentCanvas = getParentCanvas(entity);
    return parentCanvas == nullptr ? 1.0f : std::max(0.001f, parentCanvas->getScale());
}

bool isUiEntityHigher(const Entity* left, const Entity* right) {
    const int leftOrder = getUiSortingOrder(*left);
    const int rightOrder = getUiSortingOrder(*right);
    if (leftOrder != rightOrder) {
        return leftOrder > rightOrder;
    }

    if (left->getDisplayOrder() != right->getDisplayOrder()) {
        return left->getDisplayOrder() > right->getDisplayOrder();
    }

    return left->getId() > right->getId();
}
}

UISystem& UISystem::getInstance() {
    static UISystem instance;
    return instance;
}

bool UISystem::processInput(const RawInputSystem& rawInputSystem) {
    Level& level = Level::getCurrentLevel();
    PointerState pointer;

    const RawTouch* selectedTouch = nullptr;
    for (const RawTouch& touch : rawInputSystem.getTouches()) {
        if (touch.id == activeTouchId) {
            selectedTouch = &touch;
            break;
        }
    }

    if (selectedTouch == nullptr) {
        for (const RawTouch& touch : rawInputSystem.getTouches()) {
            if (touch.down) {
                selectedTouch = &touch;
                break;
            }
        }
    }

    if (selectedTouch != nullptr) {
        pointer.position = Vector2F(selectedTouch->x, selectedTouch->y);
        pointer.down = selectedTouch->down;
        pointer.pressedThisFrame = selectedTouch->down && activeTouchId < 0;
        pointer.releasedThisFrame = !selectedTouch->down && activeTouchId == selectedTouch->id;

        if (pointer.pressedThisFrame) {
            activeTouchId = selectedTouch->id;
        }
    } else {
        activeTouchId = -1;
        pointer.position = Vector2F(
            static_cast<float>(rawInputSystem.getMouseX()),
            static_cast<float>(rawInputSystem.getMouseY())
        );
        pointer.down = rawInputSystem.isMouseButtonDown(RawMouseButton::Left);
        pointer.pressedThisFrame = rawInputSystem.wasMouseButtonPressed(RawMouseButton::Left);
        pointer.releasedThisFrame = rawInputSystem.wasMouseButtonReleased(RawMouseButton::Left);
    }

    for (Entity& entity : level.getEntities()) {
        UIButtonComponent* button = entity.getComponent<UIButtonComponent>();
        if (button == nullptr) {
            continue;
        }

        button->resetFrameState();
        button->setHovered(false);
        if (entity.getId() != pressedEntityId || !pointer.down) {
            button->setPressed(false);
        }
    }

    Entity* hoveredEntity = findTopmostButtonAt(pointer.position);
    UIButtonComponent* hoveredButton =
        hoveredEntity == nullptr ? nullptr : hoveredEntity->getComponent<UIButtonComponent>();

    if (hoveredButton != nullptr) {
        hoveredButton->setHovered(true);
    }

    if (pointer.pressedThisFrame) {
        pressedEntityId = hoveredEntity == nullptr ? -1 : hoveredEntity->getId();
    }

    Entity* pressedEntity = pressedEntityId < 0 ? nullptr : level.findEntityById(pressedEntityId);
    UIButtonComponent* pressedButton =
        pressedEntity == nullptr ? nullptr : pressedEntity->getComponent<UIButtonComponent>();

    if (pressedButton != nullptr && pointer.down) {
        pressedButton->setPressed(true);
    }

    if (pointer.releasedThisFrame) {
        if (pressedButton != nullptr &&
            hoveredEntity != nullptr &&
            hoveredEntity->getId() == pressedEntityId) {
            pressedButton->setClickedThisFrame(true);

            const std::string& functionName = pressedButton->getAction();
            if (!functionName.empty()) {
                ScriptSystem::getInstance().callEntityScriptFunctionWithSelf(
                    *hoveredEntity,
                    functionName
                );
            }
        }

        if (pressedButton != nullptr) {
            pressedButton->setPressed(false);
        }

        pressedEntityId = -1;
        activeTouchId = -1;
    }

    return hoveredEntity != nullptr || pressedEntityId >= 0;
}

Entity* UISystem::findTopmostButtonAt(const Vector2F& screenPosition) {
    std::vector<Entity*> candidates;

    for (Entity& entity : Level::getCurrentLevel().getEntities()) {
        if (entity.isDestroyed() || !entity.isEnabled()) {
            continue;
        }

        UIButtonComponent* button = entity.getComponent<UIButtonComponent>();
        const RectTransformComponent* rectTransform = entity.getComponent<RectTransformComponent>();
        if (button == nullptr ||
            rectTransform == nullptr ||
            !button->isEnabled() ||
            !button->isInteractable() ||
            !rectTransform->isEnabled() ||
            !parentCanvasAllowsInput(entity)) {
            continue;
        }

        if (isPointInsideRectTransform(*rectTransform, screenPosition, getUiScale(entity))) {
            candidates.push_back(&entity);
        }
    }

    if (candidates.empty()) {
        return nullptr;
    }

    std::sort(candidates.begin(), candidates.end(), isUiEntityHigher);
    return candidates.front();
}

bool UISystem::isPointInsideRectTransform(
    const RectTransformComponent& rectTransform,
    const Vector2F& screenPosition,
    float scale
) const {
    const Vector2F& size = rectTransform.getSize();
    const Vector2F& pivot = rectTransform.getPivot();
    const float width = size.x * scale;
    const float height = size.y * scale;
    if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0f || height <= 0.0f) {
        return false;
    }

    const Vector2F localPosition = Math2D::rotate(
        screenPosition - rectTransform.getWorldPosition(),
        -rectTransform.getWorldRotation()
    );
    const float left = -(width * pivot.x);
    const float top = -(height * pivot.y);
    const float right = left + width;
    const float bottom = top + height;

    return localPosition.x >= left &&
        localPosition.x <= right &&
        localPosition.y >= top &&
        localPosition.y <= bottom;
}
