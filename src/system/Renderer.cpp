#include "system/Renderer.h"

#include "Entity.h"
#include "components/AnimationComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "misc/Level.h"
#include "system/IWindowBackend.h"

#include <algorithm>
#include <iostream>
#include <vector>

namespace {
bool rectanglesIntersect(const RenderRect& left, const RenderRect& right) {
    return left.x < right.x + right.width
        && left.x + left.width > right.x
        && left.y < right.y + right.height
        && left.y + left.height > right.y;
}

const Sprite* getRenderableSprite(const Entity& entity) {
    const AnimationComponent* animationComponent = entity.getComponent<AnimationComponent>();
    if (animationComponent != nullptr && animationComponent->getAnimation().hasFrames()) {
        return &animationComponent->getCurrentFrame();
    }

    const SpriteComponent* spriteComponent = entity.getComponent<SpriteComponent>();
    if (spriteComponent != nullptr) {
        return &spriteComponent->getSprite();
    }

    return nullptr;
}
}

Renderer& Renderer::getInstance() {
    static Renderer instance;
    return instance;
}

void Renderer::setRenderBackend(IRenderBackend* backend) {
    renderBackend = backend;
}

void Renderer::setWindowBackend(IWindowBackend* backend) {
    windowBackend = backend;
}

void Renderer::setRenderScale(float scale) {
    if (scale <= 0.0f) {
        std::cerr << "Renderer render scale must be greater than zero." << std::endl;
        return;
    }

    renderScale = scale;
}

float Renderer::getRenderScale() const {
    return renderScale;
}

Camera2D& Renderer::getCamera() {
    return camera;
}

const Camera2D& Renderer::getCamera() const {
    return camera;
}

RenderRect Renderer::getViewport() const {
    if (windowBackend == nullptr) {
        return RenderRect{};
    }

    return windowBackend->getViewport();
}

float Renderer::consumePendingPinchZoomFactor() {
    if (windowBackend == nullptr) {
        return 1.0f;
    }

    return windowBackend->consumePendingPinchZoomFactor();
}

bool Renderer::buildTileLayerBatches(LevelAsset& levelAsset) {
    return tileLayerRenderer.buildFromLevelAsset(levelAsset, renderScale);
}

void Renderer::clearTileLayerBatches() {
    tileLayerRenderer.clear();
}

void Renderer::update(float deltaTime) {
    tileLayerRenderer.update(deltaTime);
}

void Renderer::setEditorViewportActivity(
    bool dragDropActive,
    bool gridVisible,
    bool cameraActive
) {
    editorDragDropActive = dragDropActive;
    editorGridVisible = gridVisible;
    editorCameraActive = cameraActive;
}

void Renderer::updateEditorOnly(float deltaTime) {
    (void)deltaTime;
    editorViewportRenderRequested =
        editorDragDropActive
        || editorGridVisible
        || editorCameraActive;
}

bool Renderer::shouldRenderEditorViewport() const {
    return editorViewportRenderRequested;
}

bool Renderer::isEntityInViewport(const Entity& entity) const {
    if (windowBackend == nullptr || entity.isDestroyed() || !entity.isEnabled()) {
        return false;
    }

    const TransformComponent* transformComponent = entity.getComponent<TransformComponent>();
    const Sprite* engineSprite = getRenderableSprite(entity);
    if (transformComponent == nullptr || engineSprite == nullptr || engineSprite->getTextureHandle() == nullptr) {
        return false;
    }

    const RenderRect viewport = windowBackend->getViewport();
    if (viewport.width <= 0.0f || viewport.height <= 0.0f) {
        return false;
    }

    const float zoom = camera.getZoom();
    const Vector2F screenPosition = camera.worldToScreen(transformComponent->getWorldPosition(), viewport);
    const Vector2F& size = engineSprite->getSize();
    const Vector2F& origin = engineSprite->getOrigin();
    const RenderRect entityBounds{
        screenPosition.x - (origin.x * zoom),
        screenPosition.y - (origin.y * zoom),
        size.x * zoom,
        size.y * zoom
    };

    return rectanglesIntersect(entityBounds, viewport);
}

void Renderer::render() {
    if (renderBackend == nullptr || windowBackend == nullptr) {
        return;
    }

    const RenderRect viewport = windowBackend->getViewport();
    tileLayerRenderer.render(*renderBackend, camera, viewport);

    const Level& level = Level::getCurrentLevel();
    std::vector<const Entity*> renderableEntities;
    for (const Entity& entity : level.getEntities()) {
        if (entity.isDestroyed() || !entity.isEnabled()) {
            continue;
        }

        renderableEntities.push_back(&entity);
    }

    std::sort(
        renderableEntities.begin(),
        renderableEntities.end(),
        [](const Entity* left, const Entity* right) {
            if (left->getDisplayOrder() != right->getDisplayOrder()) {
                return left->getDisplayOrder() < right->getDisplayOrder();
            }

            return left->getId() < right->getId();
        }
    );

    for (const Entity* renderableEntity : renderableEntities) {
        const Entity& entity = *renderableEntity;

        const TransformComponent* transformComponent = entity.getComponent<TransformComponent>();

        if (transformComponent == nullptr) {
            continue;
        }

        const Sprite* engineSprite = getRenderableSprite(entity);
        if (engineSprite == nullptr) {
            continue;
        }

        RenderTextureHandle texture = engineSprite->getTextureHandle();

        if (texture == nullptr) {
            continue;
        }

        const Vector2F worldPosition = transformComponent->getWorldPosition();
        const Vector2F screenPosition = camera.worldToScreen(worldPosition, viewport);
        const auto& sourceRect = engineSprite->getSourceRect();
        const float zoom = camera.getZoom();

        renderBackend->drawTexture(
            texture,
            RenderRect{
                sourceRect.x,
                sourceRect.y,
                sourceRect.width,
                sourceRect.height
            },
            RenderRect{
                screenPosition.x,
                screenPosition.y,
                engineSprite->getSize().x * zoom,
                engineSprite->getSize().y * zoom
            },
            RenderVector2{
                engineSprite->getOrigin().x,
                engineSprite->getOrigin().y
            },
            transformComponent->getWorldRotation()
        );
    }
}
