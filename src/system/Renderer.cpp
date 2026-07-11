#include "system/Renderer.h"

#include "Entity.h"
#include "components/AnimationComponent.h"
#include "components/PlayerCameraComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "components/CanvasComponent.h"
#include "components/RectTransformComponent.h"
#include "components/UILabelComponent.h"
#include "components/UIButtonComponent.h"
#include "components/UIEditTextComponent.h"
#include "components/UIPanelComponent.h"
#include "math/Math2D.h"
#include "misc/Level.h"
#include "misc/RenderConstants.h"
#include "system/EngineState.h"
#include "system/IWindowBackend.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
    constexpr float Pi = 3.14159265358979323846f;

    bool rectanglesIntersect(const RenderRect &left, const RenderRect &right)
    {
        return left.x < right.x + right.width && left.x + left.width > right.x && left.y < right.y + right.height && left.y + left.height > right.y;
    }

    Vector2F rotateOffset(const Vector2F &offset, float rotationDegrees)
    {
        const float radians = rotationDegrees * Pi / 180.0f;
        const float cosine = std::cos(radians);
        const float sine = std::sin(radians);

        return Vector2F(
            (offset.x * cosine) - (offset.y * sine),
            (offset.x * sine) + (offset.y * cosine));
    }

    const Sprite *getRenderableSprite(const Entity &entity)
    {
        const AnimationComponent *animationComponent = entity.getComponent<AnimationComponent>();
        if (animationComponent != nullptr && animationComponent->getAnimation().hasFrames() && animationComponent->isEnabled())
        {
            return &animationComponent->getCurrentFrame();
        }

        const SpriteComponent *spriteComponent = entity.getComponent<SpriteComponent>();
        if (spriteComponent != nullptr && spriteComponent->isEnabled())
        {
            return &spriteComponent->getSprite();
        }

        return nullptr;
    }

    float getPositiveOrFallback(float value, float fallback)
    {
        if (!std::isfinite(value) || value <= 0.0f)
        {
            return fallback;
        }

        return value;
    }

    float clampCameraCenter(float center, float visibleSize, float minValue, float maxValue)
    {
        if (!std::isfinite(center) || !std::isfinite(visibleSize) || !std::isfinite(minValue) || !std::isfinite(maxValue) || visibleSize <= 0.0f || maxValue <= minValue)
        {
            return center;
        }

        const float boundsSize = maxValue - minValue;
        if (boundsSize <= visibleSize)
        {
            return minValue + (boundsSize * 0.5f);
        }

        const float halfVisibleSize = visibleSize * 0.5f;
        return std::clamp(center, minValue + halfVisibleSize, maxValue - halfVisibleSize);
    }

    RenderRect clipRectToViewport(const RenderRect &rect, const RenderRect &viewport)
    {
        const float left = std::max(rect.x, viewport.x);
        const float top = std::max(rect.y, viewport.y);
        const float right = std::min(rect.x + rect.width, viewport.x + viewport.width);
        const float bottom = std::min(rect.y + rect.height, viewport.y + viewport.height);

        if (right <= left || bottom <= top)
        {
            return RenderRect{};
        }

        return RenderRect{left, top, right - left, bottom - top};
    }

    RenderColor applyOpacity(RenderColor color, float opacity)
    {
        const float clampedOpacity = std::clamp(opacity, 0.0f, 1.0f);
        color.a = static_cast<std::uint8_t>(
            std::clamp(
                (static_cast<float>(color.a) * clampedOpacity) + 0.5f,
                0.0f,
                255.0f));
        return color;
    }

    RenderRect buildUiRect(const RectTransformComponent &rectTransformComponent, float scale)
    {
        const Vector2F screenPosition = rectTransformComponent.getWorldPosition();
        const Vector2F &size = rectTransformComponent.getSize();
        const Vector2F &pivot = rectTransformComponent.getPivot();
        const float clampedScale = std::isfinite(scale) ? std::max(0.0f, scale) : 1.0f;
        const float width = size.x * clampedScale;
        const float height = size.y * clampedScale;

        if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0f || height <= 0.0f)
        {
            return RenderRect{};
        }

        const float left = -(width * pivot.x);
        const float top = -(height * pivot.y);
        const float right = left + width;
        const float bottom = top + height;
        const float rotation = rectTransformComponent.getWorldRotation();
        const Vector2F corners[] = {
            screenPosition + Math2D::rotate(Vector2F(left, top), rotation),
            screenPosition + Math2D::rotate(Vector2F(right, top), rotation),
            screenPosition + Math2D::rotate(Vector2F(right, bottom), rotation),
            screenPosition + Math2D::rotate(Vector2F(left, bottom), rotation)
        };
        float minX = corners[0].x;
        float maxX = corners[0].x;
        float minY = corners[0].y;
        float maxY = corners[0].y;
        for (const Vector2F &corner : corners)
        {
            minX = std::min(minX, corner.x);
            maxX = std::max(maxX, corner.x);
            minY = std::min(minY, corner.y);
            maxY = std::max(maxY, corner.y);
        }

        return RenderRect{
            minX,
            minY,
            maxX - minX,
            maxY - minY};
    }

    const CanvasComponent *getParentCanvas(const Entity &entity)
    {
        for (const Entity *parent = entity.getParent();
             parent != nullptr;
             parent = parent->getParent())
        {
            const CanvasComponent *canvas = parent->getComponent<CanvasComponent>();
            if (canvas != nullptr)
            {
                return canvas;
            }
        }

        return nullptr;
    }

    int getUiSortingOrder(const Entity &entity)
    {
        const CanvasComponent *canvas = entity.getComponent<CanvasComponent>();
        if (canvas != nullptr)
        {
            return canvas->getSortingOrder();
        }

        const CanvasComponent *parentCanvas = getParentCanvas(entity);
        return parentCanvas == nullptr ? 0 : parentCanvas->getSortingOrder();
    }

    bool parentCanvasAllowsRender(const Entity &entity)
    {
        for (const Entity *parent = entity.getParent();
             parent != nullptr;
             parent = parent->getParent())
        {
            if (!parent->isEnabled())
            {
                return false;
            }

            const CanvasComponent *parentCanvas = parent->getComponent<CanvasComponent>();
            if (parentCanvas != nullptr && !parentCanvas->isEnabled())
            {
                return false;
            }
        }

        return true;
    }

    RenderVector2 getAlignedTextPosition(
        const RenderRect &rect,
        const UILabelComponent &label,
        unsigned int characterSize
    )
    {
        const float estimatedTextWidth =
            static_cast<float>(label.getText().size()) * static_cast<float>(characterSize) * 0.55f;
        const float estimatedTextHeight = static_cast<float>(characterSize);

        float x = rect.x;
        if (label.getHorizontalTextAlign() == HorizontalTextAlign::center)
        {
            x += std::max(0.0f, (rect.width - estimatedTextWidth) * 0.5f);
        }
        else if (label.getHorizontalTextAlign() == HorizontalTextAlign::right)
        {
            x += std::max(0.0f, rect.width - estimatedTextWidth);
        }

        float y = rect.y;
        if (label.getVerticalTextAlign() == VerticalTextAlign::center)
        {
            y += std::max(0.0f, (rect.height - estimatedTextHeight) * 0.5f);
        }
        else if (label.getVerticalTextAlign() == VerticalTextAlign::bottom)
        {
            y += std::max(0.0f, rect.height - estimatedTextHeight);
        }

        return RenderVector2{x, y};
    }

    void drawSprite(
        IRenderBackend &renderBackend,
        const Camera2D &camera,
        const RenderRect &viewport,
        const TransformComponent &transformComponent,
        const Sprite &engineSprite,
        const Vector2F &localPosition)
    {
        RenderTextureHandle texture = engineSprite.getTextureHandle();
        if (texture == nullptr)
        {
            return;
        }

        const float worldRotation = transformComponent.getWorldRotation();
        const Vector2F worldPosition =
            transformComponent.getWorldPosition() + rotateOffset(localPosition, worldRotation);
        const Vector2F screenPosition = camera.worldToScreen(worldPosition, viewport);
        const auto &sourceRect = engineSprite.getSourceRect();
        const float zoom = camera.getZoom();

        renderBackend.drawTexture(
            texture,
            RenderRect{
                sourceRect.x,
                sourceRect.y,
                sourceRect.width,
                sourceRect.height},
            RenderRect{
                screenPosition.x,
                screenPosition.y,
                engineSprite.getSize().x * zoom,
                engineSprite.getSize().y * zoom},
            RenderVector2{
                engineSprite.getOrigin().x,
                engineSprite.getOrigin().y},
            worldRotation);
    }
}

Renderer &Renderer::getInstance()
{
    static Renderer instance;
    return instance;
}

void Renderer::setRenderBackend(IRenderBackend *backend)
{
    if (cameraPreviewTarget != nullptr && renderBackend != nullptr)
    {
        renderBackend->destroyRenderTarget(cameraPreviewTarget);
        cameraPreviewTarget = nullptr;
        cameraPreviewWidth = 0;
        cameraPreviewHeight = 0;
    }

    renderBackend = backend;
}

void Renderer::setWindowBackend(IWindowBackend *backend)
{
    windowBackend = backend;
}

void Renderer::setRenderScale(float scale)
{
    if (scale <= 0.0f)
    {
        std::cerr << "Renderer render scale must be greater than zero." << std::endl;
        return;
    }

    renderScale = scale;
}

float Renderer::getRenderScale() const
{
    return renderScale;
}

Camera2D &Renderer::getCamera()
{
    return camera;
}

const Camera2D &Renderer::getCamera() const
{
    return camera;
}

RenderRect Renderer::getViewport() const
{
    if (windowBackend == nullptr)
    {
        return RenderRect{};
    }

    return windowBackend->getViewport();
}

float Renderer::getUiScale(const Entity &entity) const
{
    const CanvasComponent *canvas = entity.getComponent<CanvasComponent>();
    if (canvas != nullptr)
    {
        return std::max(0.001f, canvas->getScale());
    }

    const CanvasComponent *parentCanvas = getParentCanvas(entity);
    return parentCanvas == nullptr
        ? 1.0f
        : std::max(0.001f, parentCanvas->getScale());
}

float Renderer::consumePendingPinchZoomFactor()
{
    if (windowBackend == nullptr)
    {
        return 1.0f;
    }

    return windowBackend->consumePendingPinchZoomFactor();
}

bool Renderer::buildTileLayerBatches(LevelAsset &levelAsset)
{
    return tileLayerRenderer.buildFromLevelAsset(levelAsset, renderScale);
}

void Renderer::clearTileLayerBatches()
{
    tileLayerRenderer.clear();
}

void Renderer::update(float deltaTime)
{
    debugDrawDeltaTime = deltaTime;
    tileLayerRenderer.update(deltaTime);
}

void Renderer::setEditorViewportActivity(
    bool dragDropActive,
    bool gridVisible,
    bool cameraActive)
{
    editorDragDropActive = dragDropActive;
    editorGridVisible = gridVisible;
    editorCameraActive = cameraActive;
}

void Renderer::updateEditorOnly(float deltaTime)
{
    debugDrawDeltaTime = deltaTime;
    editorViewportRenderRequested =
        editorDragDropActive || editorGridVisible || editorCameraActive;
}

bool Renderer::shouldRenderEditorViewport() const
{
    return editorViewportRenderRequested;
}

void Renderer::applyMainCamera(const RenderRect &viewport)
{
    if (!EngineState::getInstance().isPlaying())
    {
        return;
    }

    const Level &level = Level::getCurrentLevel();
    for (const Entity &entity : level.getEntities())
    {
        if (entity.isDestroyed() || !entity.isEnabled() || entity.getTag() != "MainCamera")
        {
            continue;
        }

        const TransformComponent *transformComponent = entity.getComponent<TransformComponent>();
        const PlayerCameraComponent *cameraComponent = entity.getComponent<PlayerCameraComponent>();
        if (transformComponent == nullptr || cameraComponent == nullptr || !cameraComponent->isEnabled())
        {
            continue;
        }

        camera = buildCameraFromPlayerCamera(*transformComponent, *cameraComponent, viewport);
        return;
    }
}

void Renderer::ensureCameraPreviewTarget(int width, int height)
{
    if (renderBackend == nullptr || width <= 0 || height <= 0)
    {
        return;
    }

    if (cameraPreviewTarget != nullptr && cameraPreviewWidth == width && cameraPreviewHeight == height)
    {
        return;
    }

    if (cameraPreviewTarget != nullptr)
    {
        renderBackend->destroyRenderTarget(cameraPreviewTarget);
        cameraPreviewTarget = nullptr;
        cameraPreviewWidth = 0;
        cameraPreviewHeight = 0;
    }

    cameraPreviewTarget = renderBackend->createRenderTarget(width, height);
    if (cameraPreviewTarget != nullptr)
    {
        cameraPreviewWidth = width;
        cameraPreviewHeight = height;
    }
}

Camera2D Renderer::buildCameraFromPlayerCamera(
    const TransformComponent &transform,
    const PlayerCameraComponent &cameraComponent,
    const RenderRect &viewport) const
{
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
    if (cameraComponent.shouldClampToBounds())
    {
        center.x = clampCameraCenter(
            center.x,
            visibleWorldWidth,
            cameraComponent.getMinX(),
            cameraComponent.getMaxX());
        center.y = clampCameraCenter(
            center.y,
            visibleWorldHeight,
            cameraComponent.getMinY(),
            cameraComponent.getMaxY());
    }

    result.setPosition(Vector2F(
        center.x - (visibleWorldWidth * 0.5f),
        center.y - (visibleWorldHeight * 0.5f)));
    return result;
}

bool Renderer::isEntityInViewport(const Entity &entity) const
{
    if (windowBackend == nullptr || entity.isDestroyed() || !entity.isEnabled())
    {
        return false;
    }

    const TransformComponent *transformComponent = entity.getComponent<TransformComponent>();
    const Sprite *engineSprite = getRenderableSprite(entity);
    if (transformComponent == nullptr || engineSprite == nullptr || engineSprite->getTextureHandle() == nullptr)
    {
        return false;
    }

    const RenderRect viewport = windowBackend->getViewport();
    if (viewport.width <= 0.0f || viewport.height <= 0.0f)
    {
        return false;
    }

    const float zoom = camera.getZoom();
    const Vector2F screenPosition = camera.worldToScreen(transformComponent->getWorldPosition(), viewport);
    const Vector2F &size = engineSprite->getSize();
    const Vector2F &origin = engineSprite->getOrigin();
    const RenderRect entityBounds{
        screenPosition.x - (origin.x * zoom),
        screenPosition.y - (origin.y * zoom),
        size.x * zoom,
        size.y * zoom};

    return rectanglesIntersect(entityBounds, viewport);
}

void Renderer::debugDrawPoint(
    const Vector2F &worldPosition,
    float radius,
    RenderColor color,
    float lifetimeSeconds)
{
    if (radius <= 0.0f || !std::isfinite(radius))
    {
        return;
    }

    if (!std::isfinite(worldPosition.x) || !std::isfinite(worldPosition.y))
    {
        return;
    }

    debugPoints.push_back(DebugPoint{
        worldPosition,
        radius,
        color,
        lifetimeSeconds,
        lifetimeSeconds <= 0.0f});
}

void Renderer::clearDebugDraw()
{
    debugPoints.clear();
}

void Renderer::renderWorld(const Camera2D &renderCamera, const RenderRect &viewport)
{
    if (renderBackend == nullptr)
    {
        return;
    }

    tileLayerRenderer.render(*renderBackend, renderCamera, viewport);

    const Level &level = Level::getCurrentLevel();
    std::vector<const Entity *> renderableEntities;
    for (const Entity &entity : level.getEntities())
    {
        if (entity.isDestroyed() || !entity.isEnabled())
        {
            continue;
        }

        renderableEntities.push_back(&entity);
    }

    std::sort(
        renderableEntities.begin(),
        renderableEntities.end(),
        [](const Entity *left, const Entity *right)
        {
            if (left->getDisplayOrder() != right->getDisplayOrder())
            {
                return left->getDisplayOrder() < right->getDisplayOrder();
            }

            return left->getId() < right->getId();
        });

    for (const Entity *renderableEntity : renderableEntities)
    {
        const Entity &entity = *renderableEntity;

        const TransformComponent *transformComponent = entity.getComponent<TransformComponent>();

        if (transformComponent == nullptr)
        {
            continue;
        }

        const AnimationComponent *animationComponent = entity.getComponent<AnimationComponent>();
        if (animationComponent != nullptr && animationComponent->isEnabled() && animationComponent->getAnimation().hasFrames())
        {
            for (const AnimationFrameSprite &frameSprite : animationComponent->getCurrentFrameSprites())
            {
                drawSprite(
                    *renderBackend,
                    renderCamera,
                    viewport,
                    *transformComponent,
                    frameSprite.sprite,
                    frameSprite.localPosition);
            }
            continue;
        }

        const SpriteComponent *spriteComponent = entity.getComponent<SpriteComponent>();
        if (spriteComponent == nullptr || !spriteComponent->isEnabled())
        {
            continue;
        }

        drawSprite(
            *renderBackend,
            renderCamera,
            viewport,
            *transformComponent,
            spriteComponent->getSprite(),
            Vector2F::zero());
    }
}

void Renderer::renderDebugPoints(const Camera2D &renderCamera, const RenderRect &viewport)
{
    if (renderBackend == nullptr)
    {
        return;
    }

    for (const DebugPoint &point : debugPoints)
    {
        const Vector2F screenPosition = renderCamera.worldToScreen(point.worldPosition, viewport);
        renderBackend->drawPoint(
            RenderVector2{screenPosition.x, screenPosition.y},
            point.radius * renderCamera.getZoom(),
            point.color);
    }
}

void Renderer::updateDebugPoints()
{
    for (DebugPoint &point : debugPoints)
    {
        if (!point.oneFrame)
        {
            point.remainingSeconds -= debugDrawDeltaTime;
        }
    }

    debugPoints.erase(
        std::remove_if(
            debugPoints.begin(),
            debugPoints.end(),
            [](const DebugPoint &point)
            {
                return point.oneFrame || point.remainingSeconds <= 0.0f;
            }),
        debugPoints.end());
}

void Renderer::renderUI()
{
    if (renderBackend == nullptr || windowBackend == nullptr)
    {
        return;
    }

    const RenderRect viewport = windowBackend->getViewport();
    if (viewport.width <= 0.0f || viewport.height <= 0.0f)
    {
        return;
    }

    std::vector<const Entity *> UIEntities;

    for (const Entity &entity : Level::getCurrentLevel().getEntities())
    {
        if (entity.isDestroyed() || !entity.isEnabled())
        {
            continue;
        }

        const CanvasComponent *canvasComponent = entity.getComponent<CanvasComponent>();
        const UILabelComponent *labelComponent = entity.getComponent<UILabelComponent>();
        const UIButtonComponent *buttonComponent = entity.getComponent<UIButtonComponent>();
        const UIEditTextComponent *editTextComponent = entity.getComponent<UIEditTextComponent>();
        const UIPanelComponent *panelComponent = entity.getComponent<UIPanelComponent>();
        const RectTransformComponent *rectTransformComponent = entity.getComponent<RectTransformComponent>();
        const bool hasRenderableCanvas = canvasComponent != nullptr && canvasComponent->isEnabled();
        const bool hasRenderableLabel =
            labelComponent != nullptr &&
            labelComponent->isEnabled() &&
            parentCanvasAllowsRender(entity);
        const bool hasRenderablePanel =
            panelComponent != nullptr &&
            panelComponent->isEnabled() &&
            parentCanvasAllowsRender(entity);
        const bool hasRenderableButton =
            buttonComponent != nullptr &&
            buttonComponent->isEnabled() &&
            parentCanvasAllowsRender(entity);
        const bool hasRenderableEditText =
            editTextComponent != nullptr &&
            editTextComponent->isEnabled() &&
            parentCanvasAllowsRender(entity);

        if (rectTransformComponent != nullptr &&
            rectTransformComponent->isEnabled() &&
            (hasRenderableCanvas ||
                hasRenderableLabel ||
                hasRenderablePanel ||
                hasRenderableButton ||
                hasRenderableEditText))
        {
            UIEntities.push_back(&entity);
        }
    }

    std::sort(
        UIEntities.begin(),
        UIEntities.end(),
        [](const Entity *left, const Entity *right)
        {
            const int leftOrder = getUiSortingOrder(*left);
            const int rightOrder = getUiSortingOrder(*right);
            if (leftOrder != rightOrder)
            {
                return leftOrder < rightOrder;
            }

            if (left->getDisplayOrder() != right->getDisplayOrder())
            {
                return left->getDisplayOrder() < right->getDisplayOrder();
            }

            return left->getId() < right->getId();
        });

    for (const Entity *entity : UIEntities)
    {
        const RectTransformComponent *rectTransformComponent = entity->getComponent<RectTransformComponent>();
        const CanvasComponent *canvasComponent = entity->getComponent<CanvasComponent>();
        const UILabelComponent *labelComponent = entity->getComponent<UILabelComponent>();
        const UIButtonComponent *buttonComponent = entity->getComponent<UIButtonComponent>();
        const UIEditTextComponent *editTextComponent = entity->getComponent<UIEditTextComponent>();
        const UIPanelComponent *panelComponent = entity->getComponent<UIPanelComponent>();
        const RenderRect unclippedRect = buildUiRect(*rectTransformComponent, getUiScale(*entity));
        if (unclippedRect.width <= 0.0f || unclippedRect.height <= 0.0f)
        {
            continue;
        }

        const RenderRect clippedRect = clipRectToViewport(unclippedRect, viewport);
        if (clippedRect.width <= 0.0f || clippedRect.height <= 0.0f)
        {
            continue;
        }

        const float scale = getUiScale(*entity);
        const Vector2F position = rectTransformComponent->getWorldPosition();
        const Vector2F &size = rectTransformComponent->getSize();
        const Vector2F &pivot = rectTransformComponent->getPivot();
        const float width = size.x * scale;
        const float height = size.y * scale;
        const float rotation = rectTransformComponent->getWorldRotation();
        const RenderRect destination{position.x, position.y, width, height};
        const RenderVector2 origin{width * pivot.x, height * pivot.y};

        if (canvasComponent != nullptr && canvasComponent->isEnabled())
        {
            const RenderColor color = applyOpacity(
                canvasComponent->getCanvasColor(),
                canvasComponent->getOpacity());
            if (color.a != 0)
            {
                renderBackend->drawRect(
                    destination,
                    origin,
                    rotation,
                    color
                );
            }
        }

        if (editTextComponent != nullptr && editTextComponent->isEnabled())
        {
            const RenderColor color = editTextComponent->isFocused()
                ? editTextComponent->getFocusedColor()
                : editTextComponent->getBackgroundColor();
            if (color.a != 0)
            {
                renderBackend->drawRect(destination, origin, rotation, color);
            }
        }
        else if (buttonComponent != nullptr && buttonComponent->isEnabled())
        {
            const RenderColor color = buttonComponent->getCurrentColor();
            if (color.a != 0)
            {
                renderBackend->drawRect(destination, origin, rotation, color);
            }
        }
        else if (panelComponent != nullptr && panelComponent->isEnabled())
        {
            const RenderColor color = applyOpacity(
                panelComponent->getColor(),
                panelComponent->getOpacity()
            );
            if (color.a != 0)
            {
                renderBackend->drawRect(destination, origin, rotation, color);
            }
        }

        if (labelComponent != nullptr &&
            labelComponent->isEnabled() &&
            !labelComponent->getText().empty())
        {
            const int fontSize = std::max(1, labelComponent->getFontSize());
            const unsigned int characterSize = static_cast<unsigned int>(fontSize);
            const std::string &fontName = labelComponent->getFontName();
            const RenderVector2 localTextPosition = getAlignedTextPosition(
                RenderRect{
                    -(width * pivot.x),
                    -(height * pivot.y),
                    width,
                    height
                },
                *labelComponent,
                characterSize
            );
            const Vector2F rotatedTextPosition = Math2D::rotate(
                Vector2F(localTextPosition.x, localTextPosition.y),
                rotation
            );

            renderBackend->drawText(
                labelComponent->getText(),
                fontName.empty() ? "Assets/Fonts/default.ttf" : fontName,
                RenderVector2{
                    position.x + rotatedTextPosition.x,
                    position.y + rotatedTextPosition.y
                },
                RenderVector2{0.0f, 0.0f},
                rotation,
                characterSize,
                labelComponent->getFontColor());
        }

        if (editTextComponent != nullptr && editTextComponent->isEnabled())
        {
            const std::string displayText = editTextComponent->getDisplayText();
            const bool showPlaceholder = displayText.empty();
            const std::string visibleText = showPlaceholder
                ? editTextComponent->getPlaceholder()
                : displayText;
            const int fontSize = std::max(1, editTextComponent->getFontSize());
            const unsigned int characterSize = static_cast<unsigned int>(fontSize);
            const float padding = 8.0f;
            const std::string fontPath = editTextComponent->getFontName().empty()
                ? "Assets/Fonts/default.ttf"
                : editTextComponent->getFontName();
            const RenderVector2 localTextPosition{
                -(width * pivot.x) + padding,
                -(height * pivot.y) + std::max(0.0f, (height - static_cast<float>(characterSize)) * 0.5f)
            };
            const Vector2F rotatedTextPosition = Math2D::rotate(
                Vector2F(localTextPosition.x, localTextPosition.y),
                rotation
            );

            if (!visibleText.empty())
            {
                renderBackend->drawText(
                    visibleText,
                    fontPath,
                    RenderVector2{
                        position.x + rotatedTextPosition.x,
                        position.y + rotatedTextPosition.y
                    },
                    RenderVector2{0.0f, 0.0f},
                    rotation,
                    characterSize,
                    showPlaceholder
                        ? editTextComponent->getPlaceholderColor()
                        : editTextComponent->getTextColor()
                );
            }

            if (editTextComponent->isFocused() && !editTextComponent->isReadOnly())
            {
                const std::size_t cursorIndex = std::min(
                    editTextComponent->getCursorIndex(),
                    displayText.size()
                );
                const std::string textBeforeCursor = displayText.substr(0, cursorIndex);
                const RenderVector2 cursorTextMetrics = renderBackend->measureText(
                    textBeforeCursor,
                    fontPath,
                    characterSize
                );
                const RenderVector2 lineMetrics = renderBackend->measureText(
                    visibleText.empty() ? "M" : visibleText,
                    fontPath,
                    characterSize
                );
                const float cursorHeight =
                    lineMetrics.y > 0.0f ? lineMetrics.y : static_cast<float>(characterSize);
                const Vector2F cursorWorldPosition = position + Math2D::rotate(
                    Vector2F(
                        localTextPosition.x + cursorTextMetrics.x,
                        localTextPosition.y
                    ),
                    rotation
                );

                renderBackend->drawRect(
                    RenderRect{
                        cursorWorldPosition.x,
                        cursorWorldPosition.y,
                        2.0f,
                        cursorHeight
                    },
                    RenderVector2{0.0f, 0.0f},
                    rotation,
                    editTextComponent->getCursorColor()
                );
            }
        }
    }
}

void Renderer::render()
{
    if (renderBackend == nullptr || windowBackend == nullptr)
    {
        return;
    }

    const RenderRect viewport = windowBackend->getViewport();
    applyMainCamera(viewport);
    renderWorld(camera, viewport);
    renderDebugPoints(camera, viewport);
    updateDebugPoints();
    renderUI();

    // const Vector2F TextPosition = Vector2F(viewport.width/2, viewport.height/2);
    // Vector2F screen = camera.worldToScreen(TextPosition, viewport);

    // renderBackend->drawText(
    //     "Player",
    //     "Assets/Fonts/Silkscreen-Regular.ttf",
    //     RenderVector2{screen.x, screen.y},
    //     18,
    //     RenderColor{255, 255, 0, 255}
    // );
}

ImTextureID Renderer::renderCameraPreview(
    const TransformComponent &transform,
    const PlayerCameraComponent &cameraComponent,
    int width,
    int height)
{
    if (renderBackend == nullptr || width <= 0 || height <= 0)
    {
        return ImTextureID{};
    }

    ensureCameraPreviewTarget(width, height);
    if (cameraPreviewTarget == nullptr)
    {
        return ImTextureID{};
    }

    const RenderRect previewViewport{
        0.0f,
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height)};
    const Camera2D previewCamera =
        buildCameraFromPlayerCamera(transform, cameraComponent, previewViewport);

    renderBackend->beginRenderTarget(cameraPreviewTarget);
    renderBackend->clear(RenderConstants::ClearColor);
    renderWorld(previewCamera, previewViewport);
    renderBackend->endRenderTarget();

    RenderTextureHandle texture =
        renderBackend->getRenderTargetTexture(cameraPreviewTarget);
    if (texture == nullptr)
    {
        return ImTextureID{};
    }

    return renderBackend->getImGuiTextureId(texture);
}

bool Renderer::pickScreenColor(int x, int y, RenderColor &outColor)
{
    if (renderBackend == nullptr)
    {
        return false;
    }

    return renderBackend->readScreenPixel(x, y, outColor);
}
