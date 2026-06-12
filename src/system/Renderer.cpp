#include "system/Renderer.h"

#include "components/AnimationComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "misc/Level.h"
#include "misc/LevelAsset.h"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Rect.hpp>

#include <cmath>
#include <iostream>

namespace {
const TilesetInfo* findTilesetForGid(const LevelAsset& levelAsset, int gid) {
    const TilesetInfo* result = nullptr;

    for (const TilesetInfo& tileset : levelAsset.getTilesets()) {
        if (gid >= tileset.firstGid) {
            result = &tileset;
        }
    }

    return result;
}
}

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
    clearTileLayerBatches();

    if (!levelAsset.isLoaded() && !levelAsset.load()) {
        std::cerr << "Renderer failed to build tile batches. Could not load level asset: "
                  << levelAsset.getPath() << std::endl;
        return false;
    }

    const float scaledTileWidth = static_cast<float>(levelAsset.getTileWidth()) * renderScale;
    const float scaledTileHeight = static_cast<float>(levelAsset.getTileHeight()) * renderScale;
    const unsigned int batchWidth = static_cast<unsigned int>(
        std::ceil(static_cast<float>(levelAsset.getMapWidth()) * scaledTileWidth)
    );
    const unsigned int batchHeight = static_cast<unsigned int>(
        std::ceil(static_cast<float>(levelAsset.getMapHeight()) * scaledTileHeight)
    );

    if (batchWidth == 0 || batchHeight == 0) {
        std::cerr << "Renderer failed to build tile batches. Invalid map size." << std::endl;
        return false;
    }

    const auto createTileDrawInfo = [scaledTileWidth, scaledTileHeight](
        const TilesetInfo& tileset,
        int localTileId,
        int tileX,
        int tileY
    ) {
        TileDrawInfo drawInfo;
        drawInfo.texture = tileset.textureAsset->getTexture();
        drawInfo.sourceX = (localTileId % tileset.columns) * tileset.tileWidth;
        drawInfo.sourceY = (localTileId / tileset.columns) * tileset.tileHeight;
        drawInfo.sourceWidth = tileset.tileWidth;
        drawInfo.sourceHeight = tileset.tileHeight;
        drawInfo.destinationX = static_cast<float>(tileX) * scaledTileWidth;
        drawInfo.destinationY = static_cast<float>(tileY) * scaledTileHeight;
        drawInfo.destinationWidth = scaledTileWidth;
        drawInfo.destinationHeight = scaledTileHeight;
        return drawInfo;
    };

    for (const LevelGroupInfo& group : levelAsset.getGroups()) {
        for (const TileLayerInfo& tileLayer : group.tileLayers) {
            if (!tileLayer.visible) {
                continue;
            }

            TileLayerBatch batch;
            batch.groupName = group.name;
            batch.layerName = tileLayer.name;

            if (!batch.renderTexture.resize({batchWidth, batchHeight})) {
                std::cerr << "Renderer failed to create tile batch texture for group "
                          << group.name << ", layer " << tileLayer.name << std::endl;
                continue;
            }

            for (int y = 0; y < tileLayer.height; ++y) {
                for (int x = 0; x < tileLayer.width; ++x) {
                    const int index = y * tileLayer.width + x;
                    if (index < 0 || static_cast<std::size_t>(index) >= tileLayer.gids.size()) {
                        continue;
                    }

                    const int gid = tileLayer.gids[static_cast<std::size_t>(index)];
                    if (gid == 0) {
                        continue;
                    }

                    const TilesetInfo* tileset = findTilesetForGid(levelAsset, gid);
                    if (tileset == nullptr
                        || tileset->columns == 0
                        || tileset->textureAsset == nullptr
                        || tileset->textureAsset->getTexture() == nullptr) {
                        continue;
                    }

                    const int localTileId = gid - tileset->firstGid;
                    const auto animationIterator = tileset->animations.find(localTileId);

                    if (animationIterator == tileset->animations.end()) {
                        batch.staticTiles.push_back(createTileDrawInfo(*tileset, localTileId, x, y));
                        continue;
                    }

                    AnimatedTileDrawInfo animatedTile;
                    for (const TileAnimationFrame& frame : animationIterator->second.frames) {
                        AnimatedTileFrame animatedFrame;
                        animatedFrame.drawInfo = createTileDrawInfo(*tileset, frame.tileId, x, y);
                        animatedFrame.durationSeconds = frame.durationSeconds;
                        animatedTile.frames.push_back(animatedFrame);
                    }

                    if (!animatedTile.frames.empty()) {
                        batch.animatedTiles.push_back(std::move(animatedTile));
                    }
                }
            }

            rebuildTileLayerBatch(batch);

            std::cout << "Renderer batched group \"" << group.name
                      << "\" layer \"" << tileLayer.name << "\" into "
                      << batchWidth << "x" << batchHeight << " texture"
                      << " (staticTiles=" << batch.staticTiles.size()
                      << ", animatedTiles=" << batch.animatedTiles.size()
                      << ")" << std::endl;

            tileLayerBatches.push_back(std::move(batch));
        }
    }

    return true;
}

void Renderer::clearTileLayerBatches() {
    tileLayerBatches.clear();
}

void Renderer::update(float deltaTime) {
    for (TileLayerBatch& batch : tileLayerBatches) {
        for (AnimatedTileDrawInfo& animatedTile : batch.animatedTiles) {
            if (animatedTile.frames.empty()) {
                continue;
            }

            animatedTile.elapsedTime += deltaTime;

            bool frameChanged = false;
            float currentDuration = animatedTile.frames[animatedTile.currentFrameIndex].durationSeconds;
            while (currentDuration > 0.0f && animatedTile.elapsedTime >= currentDuration) {
                animatedTile.elapsedTime -= currentDuration;
                animatedTile.currentFrameIndex = (animatedTile.currentFrameIndex + 1) % animatedTile.frames.size();
                currentDuration = animatedTile.frames[animatedTile.currentFrameIndex].durationSeconds;
                frameChanged = true;
            }

            if (frameChanged) {
                batch.dirty = true;
            }
        }
    }
}

void Renderer::rebuildTileLayerBatch(TileLayerBatch& batch) {
    const auto drawTile = [&batch](const TileDrawInfo& drawInfo) {
        if (drawInfo.texture == nullptr || drawInfo.sourceWidth == 0 || drawInfo.sourceHeight == 0) {
            return;
        }

        sf::Sprite tileSprite(
            *drawInfo.texture,
            sf::IntRect(
                {drawInfo.sourceX, drawInfo.sourceY},
                {drawInfo.sourceWidth, drawInfo.sourceHeight}
            )
        );

        tileSprite.setPosition({drawInfo.destinationX, drawInfo.destinationY});
        tileSprite.setScale({
            drawInfo.destinationWidth / static_cast<float>(drawInfo.sourceWidth),
            drawInfo.destinationHeight / static_cast<float>(drawInfo.sourceHeight)
        });

        batch.renderTexture.draw(tileSprite);
    };

    batch.renderTexture.clear(sf::Color::Transparent);

    for (const TileDrawInfo& staticTile : batch.staticTiles) {
        drawTile(staticTile);
    }

    for (const AnimatedTileDrawInfo& animatedTile : batch.animatedTiles) {
        if (animatedTile.frames.empty()) {
            continue;
        }

        drawTile(animatedTile.frames[animatedTile.currentFrameIndex].drawInfo);
    }

    batch.renderTexture.display();
    batch.dirty = false;
}

void Renderer::render() {
    if (window == nullptr) {
        return;
    }

    for (TileLayerBatch& batch : tileLayerBatches) {
        if (batch.dirty) {
            rebuildTileLayerBatch(batch);
        }

        sf::Sprite batchSprite(batch.renderTexture.getTexture());
        window->draw(batchSprite);
    }

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
