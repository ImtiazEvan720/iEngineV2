#include "editor/components/ComponentDrawerRegistry.h"

#include "components/AnimationComponent.h"
#include "components/CanvasComponent.h"
#include "components/CollisionComponent.h"
#include "components/Component.h"
#include "components/PlayerCameraComponent.h"
#include "components/PlayerController.h"
#include "components/RectTransformComponent.h"
#include "components/ScriptComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "components/UILabelComponent.h"
#include "components/UIButtonComponent.h"
#include "components/UIPanelComponent.h"
#include "editor/components/AnimationComponentDrawer.h"
#include "editor/components/CanvasComponentDrawer.h"
#include "editor/components/CollisionComponentDrawer.h"
#include "editor/components/IComponentDrawer.h"
#include "editor/components/PlayerCameraComponentDrawer.h"
#include "editor/components/PlayerControllerComponentDrawer.h"
#include "editor/components/RectTransformComponentDrawer.h"
#include "editor/components/ScriptComponentDrawer.h"
#include "editor/components/SpriteComponentDrawer.h"
#include "editor/components/TransformComponentDrawer.h"
#include "editor/components/UILabelComponentDrawer.h"
#include "editor/components/UIButtonComponentDrawer.h"
#include "editor/components/UIPanelComponentDrawer.h"

#include <typeinfo>
#include <utility>

ComponentDrawerRegistry::ComponentDrawerRegistry() {
    registerDrawer<TransformComponent>(
        "TransformComponent",
        std::make_unique<TransformComponentDrawer>()
    );
    registerDrawer<SpriteComponent>(
        "SpriteComponent",
        std::make_unique<SpriteComponentDrawer>()
    );
    registerDrawer<AnimationComponent>(
        "AnimationComponent",
        std::make_unique<AnimationComponentDrawer>()
    );
    registerDrawer<CollisionComponent>(
        "CollisionComponent",
        std::make_unique<CollisionComponentDrawer>()
    );
    registerDrawer<PlayerCameraComponent>(
        "PlayerCameraComponent",
        std::make_unique<PlayerCameraComponentDrawer>()
    );
    registerDrawer<ScriptComponent>(
        "ScriptComponent",
        std::make_unique<ScriptComponentDrawer>()
    );
    registerDrawer<CanvasComponent>(
        "CanvasComponent",
        std::make_unique<CanvasComponentDrawer>()
    );
    registerDrawer<UILabelComponent>(
        "UILabelComponent",
        std::make_unique<UILabelComponentDrawer>()
    );
    registerDrawer<UIButtonComponent>(
        "UIButtonComponent",
        std::make_unique<UIButtonComponentDrawer>()
    );
    registerDrawer<UIPanelComponent>(
        "UIPanelComponent",
        std::make_unique<UIPanelComponentDrawer>()
    );
    registerDrawer<RectTransformComponent>(
        "RectTransformComponent",
        std::make_unique<RectTransformComponentDrawer>()
    );
    registerDrawer<PlayerController>(
        "PlayerController",
        std::make_unique<PlayerControllerComponentDrawer>()
    );
}

IComponentDrawer* ComponentDrawerRegistry::getDrawer(Component& component) {
    const auto iterator = drawers.find(std::type_index(typeid(component)));
    if (iterator == drawers.end()) {
        return nullptr;
    }

    return iterator->second.drawer.get();
}

const char* ComponentDrawerRegistry::getComponentName(Component& component) const {
    const auto iterator = drawers.find(std::type_index(typeid(component)));
    if (iterator == drawers.end()) {
        return nullptr;
    }

    return iterator->second.name.c_str();
}
