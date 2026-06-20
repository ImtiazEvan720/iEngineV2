#include "system/Renderer.h"

#include "components/AnimationComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "misc/Level.h"
#include "system/IWindowBackend.h"

#include <iostream>

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

void Renderer::render() {
    if (renderBackend == nullptr || windowBackend == nullptr) {
        return;
    }

    const RenderRect viewport = windowBackend->getViewport();
    tileLayerRenderer.render(*renderBackend, camera, viewport);

    const Level& level = Level::getCurrentLevel();
    for (const Entity& entity : level.getEntities()) {
        if (entity.isDestroyed() || !entity.isEnabled()) {
            continue;
        }

        const AnimationComponent* animationComponent = entity.getComponent<AnimationComponent>();
        const SpriteComponent* spriteComponent = entity.getComponent<SpriteComponent>();
        const TransformComponent* transformComponent = entity.getComponent<TransformComponent>();

        if (transformComponent == nullptr) {
            continue;
        }

        const Sprite* engineSprite = nullptr;
        if (animationComponent != nullptr && animationComponent->getAnimation().hasFrames()) {
            engineSprite = &animationComponent->getCurrentFrame();
        } else if (spriteComponent != nullptr) {
            engineSprite = &spriteComponent->getSprite();
        }

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
