#include "system/Renderer.h"

#include "Entity.h"
#include "components/AnimationComponent.h"
#include "components/PlayerCameraComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "misc/Level.h"
#include "misc/RenderConstants.h"
#include "system/EngineState.h"
#include "system/IWindowBackend.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace {
constexpr float Pi = 3.14159265358979323846f;

bool rectanglesIntersect(const RenderRect& left, const RenderRect& right) {
    return left.x < right.x + right.width
        && left.x + left.width > right.x
        && left.y < right.y + right.height
        && left.y + left.height > right.y;
}

Vector2F rotateOffset(const Vector2F& offset, float rotationDegrees) {
    const float radians = rotationDegrees * Pi / 180.0f;
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);

    return Vector2F(
        (offset.x * cosine) - (offset.y * sine),
        (offset.x * sine) + (offset.y * cosine)
    );
}

const Sprite* getRenderableSprite(const Entity& entity) {
    const AnimationComponent* animationComponent = entity.getComponent<AnimationComponent>();
    if (animationComponent != nullptr && animationComponent->getAnimation().hasFrames() && animationComponent->isEnabled()) {
        return &animationComponent->getCurrentFrame();
    }

    const SpriteComponent* spriteComponent = entity.getComponent<SpriteComponent>();
    if (spriteComponent != nullptr && spriteComponent->isEnabled()) {
        return &spriteComponent->getSprite();
    }

    return nullptr;
}

float getPositiveOrFallback(float value, float fallback) {
    if (!std::isfinite(value) || value <= 0.0f) {
        return fallback;
    }

    return value;
}

float clampCameraCenter(float center, float visibleSize, float minValue, float maxValue) {
    if (!std::isfinite(center)
        || !std::isfinite(visibleSize)
        || !std::isfinite(minValue)
        || !std::isfinite(maxValue)
        || visibleSize <= 0.0f
        || maxValue <= minValue) {
        return center;
    }

    const float boundsSize = maxValue - minValue;
    if (boundsSize <= visibleSize) {
        return minValue + (boundsSize * 0.5f);
    }

    const float halfVisibleSize = visibleSize * 0.5f;
    return std::clamp(center, minValue + halfVisibleSize, maxValue - halfVisibleSize);
}

void drawSprite(
    IRenderBackend& renderBackend,
    const Camera2D& camera,
    const RenderRect& viewport,
    const TransformComponent& transformComponent,
    const Sprite& engineSprite,
    const Vector2F& localPosition
) {
    RenderTextureHandle texture = engineSprite.getTextureHandle();
    if (texture == nullptr) {
        return;
    }

    const float worldRotation = transformComponent.getWorldRotation();
    const Vector2F worldPosition =
        transformComponent.getWorldPosition() + rotateOffset(localPosition, worldRotation);
    const Vector2F screenPosition = camera.worldToScreen(worldPosition, viewport);
    const auto& sourceRect = engineSprite.getSourceRect();
    const float zoom = camera.getZoom();

    renderBackend.drawTexture(
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
            engineSprite.getSize().x * zoom,
            engineSprite.getSize().y * zoom
        },
        RenderVector2{
            engineSprite.getOrigin().x,
            engineSprite.getOrigin().y
        },
        worldRotation
    );
}
}

Renderer& Renderer::getInstance() {
    static Renderer instance;
    return instance;
}

void Renderer::setRenderBackend(IRenderBackend* backend) {
    if (cameraPreviewTarget != nullptr && renderBackend != nullptr) {
        renderBackend->destroyRenderTarget(cameraPreviewTarget);
        cameraPreviewTarget = nullptr;
        cameraPreviewWidth = 0;
        cameraPreviewHeight = 0;
    }

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
    debugDrawDeltaTime = deltaTime;
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
    debugDrawDeltaTime = deltaTime;
    editorViewportRenderRequested =
        editorDragDropActive
        || editorGridVisible
        || editorCameraActive;
}

bool Renderer::shouldRenderEditorViewport() const {
    return editorViewportRenderRequested;
}

void Renderer::applyMainCamera(const RenderRect& viewport) {
    if (!EngineState::getInstance().isPlaying()) {
        return;
    }

    const Level& level = Level::getCurrentLevel();
    for (const Entity& entity : level.getEntities()) {
        if (entity.isDestroyed() || !entity.isEnabled() || entity.getTag() != "MainCamera") {
            continue;
        }

        const TransformComponent* transformComponent = entity.getComponent<TransformComponent>();
        const PlayerCameraComponent* cameraComponent = entity.getComponent<PlayerCameraComponent>();
        if (transformComponent == nullptr
            || cameraComponent == nullptr
            || !cameraComponent->isEnabled()) {
            continue;
        }

        camera = buildCameraFromPlayerCamera(*transformComponent, *cameraComponent, viewport);
        return;
    }
}

void Renderer::ensureCameraPreviewTarget(int width, int height) {
    if (renderBackend == nullptr || width <= 0 || height <= 0) {
        return;
    }

    if (cameraPreviewTarget != nullptr
        && cameraPreviewWidth == width
        && cameraPreviewHeight == height) {
        return;
    }

    if (cameraPreviewTarget != nullptr) {
        renderBackend->destroyRenderTarget(cameraPreviewTarget);
        cameraPreviewTarget = nullptr;
        cameraPreviewWidth = 0;
        cameraPreviewHeight = 0;
    }

    cameraPreviewTarget = renderBackend->createRenderTarget(width, height);
    if (cameraPreviewTarget != nullptr) {
        cameraPreviewWidth = width;
        cameraPreviewHeight = height;
    }
}

Camera2D Renderer::buildCameraFromPlayerCamera(
    const TransformComponent& transform,
    const PlayerCameraComponent& cameraComponent,
    const RenderRect& viewport
) const {
    Camera2D result;
    result.setZoom(cameraComponent.getZoom());

    const float zoom = result.getZoom();
    const float viewportWidth =
        getPositiveOrFallback(cameraComponent.getViewportWidth(), viewport.width);
    const float viewportHeight =
        getPositiveOrFallback(cameraComponent.getViewportHeight(), viewport.height);
    const float visibleWorldWidth = viewportWidth / zoom;
    const float visibleWorldHeight = viewportHeight / zoom;

    Vector2F center = transform.getWorldPosition() + cameraComponent.getOffset();
    if (cameraComponent.shouldClampToBounds()) {
        center.x = clampCameraCenter(
            center.x,
            visibleWorldWidth,
            cameraComponent.getMinX(),
            cameraComponent.getMaxX()
        );
        center.y = clampCameraCenter(
            center.y,
            visibleWorldHeight,
            cameraComponent.getMinY(),
            cameraComponent.getMaxY()
        );
    }

    result.setPosition(Vector2F(
        center.x - (visibleWorldWidth * 0.5f),
        center.y - (visibleWorldHeight * 0.5f)
    ));
    return result;
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

void Renderer::debugDrawPoint(
    const Vector2F& worldPosition,
    float radius,
    RenderColor color,
    float lifetimeSeconds
) {
    if (radius <= 0.0f || !std::isfinite(radius)) {
        return;
    }

    if (!std::isfinite(worldPosition.x) || !std::isfinite(worldPosition.y)) {
        return;
    }

    debugPoints.push_back(DebugPoint{
        worldPosition,
        radius,
        color,
        lifetimeSeconds,
        lifetimeSeconds <= 0.0f
    });
}

void Renderer::clearDebugDraw() {
    debugPoints.clear();
}

void Renderer::renderWorld(const Camera2D& renderCamera, const RenderRect& viewport) {
    if (renderBackend == nullptr) {
        return;
    }

    tileLayerRenderer.render(*renderBackend, renderCamera, viewport);

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

        const AnimationComponent* animationComponent = entity.getComponent<AnimationComponent>();
        if (animationComponent != nullptr
            && animationComponent->isEnabled()
            && animationComponent->getAnimation().hasFrames()) {
            for (const AnimationFrameSprite& frameSprite : animationComponent->getCurrentFrameSprites()) {
                drawSprite(
                    *renderBackend,
                    renderCamera,
                    viewport,
                    *transformComponent,
                    frameSprite.sprite,
                    frameSprite.localPosition
                );
            }
            continue;
        }

        const SpriteComponent* spriteComponent = entity.getComponent<SpriteComponent>();
        if (spriteComponent == nullptr || !spriteComponent->isEnabled()) {
            continue;
        }

        drawSprite(
            *renderBackend,
            renderCamera,
            viewport,
            *transformComponent,
            spriteComponent->getSprite(),
            Vector2F::zero()
        );
    }
}

void Renderer::renderDebugPoints(const Camera2D& renderCamera, const RenderRect& viewport) {
    if (renderBackend == nullptr) {
        return;
    }

    for (const DebugPoint& point : debugPoints) {
        const Vector2F screenPosition = renderCamera.worldToScreen(point.worldPosition, viewport);
        renderBackend->drawPoint(
            RenderVector2{screenPosition.x, screenPosition.y},
            point.radius * renderCamera.getZoom(),
            point.color
        );
    }
}

void Renderer::updateDebugPoints() {
    for (DebugPoint& point : debugPoints) {
        if (!point.oneFrame) {
            point.remainingSeconds -= debugDrawDeltaTime;
        }
    }

    debugPoints.erase(
        std::remove_if(
            debugPoints.begin(),
            debugPoints.end(),
            [](const DebugPoint& point) {
                return point.oneFrame || point.remainingSeconds <= 0.0f;
            }
        ),
        debugPoints.end()
    );
}

void Renderer::render() {
    if (renderBackend == nullptr || windowBackend == nullptr) {
        return;
    }

    const RenderRect viewport = windowBackend->getViewport();
    applyMainCamera(viewport);
    renderWorld(camera, viewport);
    renderDebugPoints(camera, viewport);
    updateDebugPoints();
}

ImTextureID Renderer::renderCameraPreview(
    const TransformComponent& transform,
    const PlayerCameraComponent& cameraComponent,
    int width,
    int height
) {
    if (renderBackend == nullptr || width <= 0 || height <= 0) {
        return ImTextureID{};
    }

    ensureCameraPreviewTarget(width, height);
    if (cameraPreviewTarget == nullptr) {
        return ImTextureID{};
    }

    const RenderRect previewViewport{
        0.0f,
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height)
    };
    const Camera2D previewCamera =
        buildCameraFromPlayerCamera(transform, cameraComponent, previewViewport);

    renderBackend->beginRenderTarget(cameraPreviewTarget);
    renderBackend->clear(RenderConstants::ClearColor);
    renderWorld(previewCamera, previewViewport);
    renderBackend->endRenderTarget();

    RenderTextureHandle texture =
        renderBackend->getRenderTargetTexture(cameraPreviewTarget);
    if (texture == nullptr) {
        return ImTextureID{};
    }

    return renderBackend->getImGuiTextureId(texture);
}
