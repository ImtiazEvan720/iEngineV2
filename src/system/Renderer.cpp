#include "system/Renderer.h"

#include "components/AnimationComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "misc/Level.h"

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <iostream>

Renderer& Renderer::getInstance() {
    static Renderer instance;
    return instance;
}

void Renderer::setWindow(sf::RenderWindow* renderWindow) {
    window = renderWindow;
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

bool Renderer::buildTileLayerBatches(LevelAsset& levelAsset) {
    return tileLayerRenderer.buildFromLevelAsset(levelAsset, renderScale);
}

void Renderer::clearTileLayerBatches() {
    tileLayerRenderer.clear();
}

void Renderer::update(float deltaTime) {
    tileLayerRenderer.update(deltaTime);
}

void Renderer::render() {
    if (window == nullptr) {
        return;
    }

    tileLayerRenderer.render(*window);

    const Level& level = Level::getCurrentLevel();
    for (const Entity& entity : level.getEntities()) {
        if (entity.isDestroyed()) {
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

        sf::Texture* texture = engineSprite->getTexture();

        if (texture == nullptr) {
            continue;
        }

        sf::Sprite sfmlSprite(*texture);
        sfmlSprite.setTextureRect(sf::IntRect(engineSprite->getSourceRect()));
        sfmlSprite.setOrigin({engineSprite->getOrigin().x, engineSprite->getOrigin().y});
        const Vector2F worldPosition = transformComponent->getWorldPosition();
        sfmlSprite.setPosition({worldPosition.x, worldPosition.y});
        sfmlSprite.setRotation(sf::degrees(transformComponent->getWorldRotation()));

        const sf::FloatRect sourceRect = engineSprite->getSourceRect();
        if (sourceRect.size.x != 0.0f && sourceRect.size.y != 0.0f) {
            sfmlSprite.setScale({
                engineSprite->getSize().x / sourceRect.size.x,
                engineSprite->getSize().y / sourceRect.size.y
            });
        }

        window->draw(sfmlSprite);
    }
}
