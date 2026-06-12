#include "system/TileLayerRenderer.h"

#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Vertex.hpp>

#include <iostream>
#include <utility>

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

sf::IntRect getTileSourceRect(const TilesetInfo& tileset, int localTileId) {
    const int sourceX = (localTileId % tileset.columns) * tileset.tileWidth;
    const int sourceY = (localTileId / tileset.columns) * tileset.tileHeight;

    return sf::IntRect(
        {sourceX, sourceY},
        {tileset.tileWidth, tileset.tileHeight}
    );
}
}

bool TileLayerRenderer::buildFromLevelAsset(LevelAsset& levelAsset, float renderScale) {
    clear();

    if (renderScale <= 0.0f) {
        std::cerr << "TileLayerRenderer render scale must be greater than zero." << std::endl;
        return false;
    }

    if (!levelAsset.isLoaded() && !levelAsset.load()) {
        std::cerr << "TileLayerRenderer failed to load level asset: "
                  << levelAsset.getPath() << std::endl;
        return false;
    }

    const float scaledTileWidth = static_cast<float>(levelAsset.getTileWidth()) * renderScale;
    const float scaledTileHeight = static_cast<float>(levelAsset.getTileHeight()) * renderScale;

    if (scaledTileWidth <= 0.0f || scaledTileHeight <= 0.0f) {
        std::cerr << "TileLayerRenderer failed to build tile chunks. Invalid tile size." << std::endl;
        return false;
    }

    for (const LevelGroupInfo& group : levelAsset.getGroups()) {
        for (const TileLayerInfo& tileLayer : group.tileLayers) {
            if (!tileLayer.visible) {
                continue;
            }

            RenderLayer renderLayer;
            renderLayer.groupName = group.name;
            renderLayer.layerName = tileLayer.name;

            std::size_t staticTileCount = 0;
            std::size_t animatedTileCount = 0;

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
                    const int chunkX = x / chunkSize;
                    const int chunkY = y / chunkSize;
                    TileChunk& chunk = getOrCreateChunk(
                        renderLayer,
                        chunkX,
                        chunkY,
                        tileset->textureAsset->getTexture()
                    );

                    const float destinationX = static_cast<float>(x) * scaledTileWidth;
                    const float destinationY = static_cast<float>(y) * scaledTileHeight;
                    const std::size_t firstVertexIndex = appendTileQuad(
                        chunk.vertices,
                        destinationX,
                        destinationY,
                        scaledTileWidth,
                        scaledTileHeight,
                        getTileSourceRect(*tileset, localTileId)
                    );

                    const auto animationIterator = tileset->animations.find(localTileId);
                    if (animationIterator == tileset->animations.end()) {
                        ++staticTileCount;
                        continue;
                    }

                    AnimatedTileRef animatedTile;
                    animatedTile.firstVertexIndex = firstVertexIndex;
                    animatedTile.tileset = tileset;
                    animatedTile.animation = &animationIterator->second;
                    chunk.animatedTiles.push_back(animatedTile);
                    ++animatedTileCount;
                }
            }

            std::cout << "TileLayerRenderer built group \"" << renderLayer.groupName
                      << "\" layer \"" << renderLayer.layerName
                      << "\" chunks=" << renderLayer.chunks.size()
                      << " staticTiles=" << staticTileCount
                      << " animatedTiles=" << animatedTileCount << std::endl;

            layers.push_back(std::move(renderLayer));
        }
    }

    return true;
}

void TileLayerRenderer::clear() {
    layers.clear();
}

void TileLayerRenderer::update(float deltaTime) {
    for (RenderLayer& layer : layers) {
        for (TileChunk& chunk : layer.chunks) {
            for (AnimatedTileRef& animatedTile : chunk.animatedTiles) {
                if (animatedTile.tileset == nullptr
                    || animatedTile.animation == nullptr
                    || animatedTile.animation->frames.empty()) {
                    continue;
                }

                animatedTile.elapsedTime += deltaTime;

                bool frameChanged = false;
                float currentDuration = animatedTile.animation->frames[animatedTile.currentFrameIndex].durationSeconds;
                while (currentDuration > 0.0f && animatedTile.elapsedTime >= currentDuration) {
                    animatedTile.elapsedTime -= currentDuration;
                    animatedTile.currentFrameIndex = (animatedTile.currentFrameIndex + 1) % animatedTile.animation->frames.size();
                    currentDuration = animatedTile.animation->frames[animatedTile.currentFrameIndex].durationSeconds;
                    frameChanged = true;
                }

                if (!frameChanged) {
                    continue;
                }

                const int frameTileId = animatedTile.animation->frames[animatedTile.currentFrameIndex].tileId;
                updateTileTexCoords(
                    chunk.vertices,
                    animatedTile.firstVertexIndex,
                    getTileSourceRect(*animatedTile.tileset, frameTileId)
                );
            }
        }
    }
}

void TileLayerRenderer::render(sf::RenderWindow& window) const {
    for (const RenderLayer& layer : layers) {
        for (const TileChunk& chunk : layer.chunks) {
            if (chunk.texture == nullptr || chunk.vertices.getVertexCount() == 0) {
                continue;
            }

            sf::RenderStates states;
            states.texture = chunk.texture;
            window.draw(chunk.vertices, states);
        }
    }
}

TileLayerRenderer::TileChunk& TileLayerRenderer::getOrCreateChunk(
    RenderLayer& renderLayer,
    int chunkX,
    int chunkY,
    sf::Texture* texture
) {
    for (TileChunk& chunk : renderLayer.chunks) {
        if (chunk.chunkX == chunkX && chunk.chunkY == chunkY && chunk.texture == texture) {
            return chunk;
        }
    }

    renderLayer.chunks.emplace_back();
    TileChunk& chunk = renderLayer.chunks.back();
    chunk.chunkX = chunkX;
    chunk.chunkY = chunkY;
    chunk.texture = texture;
    chunk.vertices.setPrimitiveType(sf::PrimitiveType::Triangles);
    return chunk;
}

std::size_t TileLayerRenderer::appendTileQuad(
    sf::VertexArray& vertices,
    float destinationX,
    float destinationY,
    float destinationWidth,
    float destinationHeight,
    const sf::IntRect& sourceRect
) const {
    const sf::Vector2f topLeft(destinationX, destinationY);
    const sf::Vector2f topRight(destinationX + destinationWidth, destinationY);
    const sf::Vector2f bottomRight(destinationX + destinationWidth, destinationY + destinationHeight);
    const sf::Vector2f bottomLeft(destinationX, destinationY + destinationHeight);

    const sf::Vector2f uvTopLeft(
        static_cast<float>(sourceRect.position.x),
        static_cast<float>(sourceRect.position.y)
    );
    const sf::Vector2f uvTopRight(
        static_cast<float>(sourceRect.position.x + sourceRect.size.x),
        static_cast<float>(sourceRect.position.y)
    );
    const sf::Vector2f uvBottomRight(
        static_cast<float>(sourceRect.position.x + sourceRect.size.x),
        static_cast<float>(sourceRect.position.y + sourceRect.size.y)
    );
    const sf::Vector2f uvBottomLeft(
        static_cast<float>(sourceRect.position.x),
        static_cast<float>(sourceRect.position.y + sourceRect.size.y)
    );

    const std::size_t firstVertexIndex = vertices.getVertexCount();

    vertices.append(sf::Vertex{topLeft, sf::Color::White, uvTopLeft});
    vertices.append(sf::Vertex{topRight, sf::Color::White, uvTopRight});
    vertices.append(sf::Vertex{bottomRight, sf::Color::White, uvBottomRight});

    vertices.append(sf::Vertex{topLeft, sf::Color::White, uvTopLeft});
    vertices.append(sf::Vertex{bottomRight, sf::Color::White, uvBottomRight});
    vertices.append(sf::Vertex{bottomLeft, sf::Color::White, uvBottomLeft});

    return firstVertexIndex;
}

void TileLayerRenderer::updateTileTexCoords(
    sf::VertexArray& vertices,
    std::size_t firstVertexIndex,
    const sf::IntRect& sourceRect
) const {
    if (firstVertexIndex + 5 >= vertices.getVertexCount()) {
        return;
    }

    const sf::Vector2f uvTopLeft(
        static_cast<float>(sourceRect.position.x),
        static_cast<float>(sourceRect.position.y)
    );
    const sf::Vector2f uvTopRight(
        static_cast<float>(sourceRect.position.x + sourceRect.size.x),
        static_cast<float>(sourceRect.position.y)
    );
    const sf::Vector2f uvBottomRight(
        static_cast<float>(sourceRect.position.x + sourceRect.size.x),
        static_cast<float>(sourceRect.position.y + sourceRect.size.y)
    );
    const sf::Vector2f uvBottomLeft(
        static_cast<float>(sourceRect.position.x),
        static_cast<float>(sourceRect.position.y + sourceRect.size.y)
    );

    vertices[firstVertexIndex + 0].texCoords = uvTopLeft;
    vertices[firstVertexIndex + 1].texCoords = uvTopRight;
    vertices[firstVertexIndex + 2].texCoords = uvBottomRight;

    vertices[firstVertexIndex + 3].texCoords = uvTopLeft;
    vertices[firstVertexIndex + 4].texCoords = uvBottomRight;
    vertices[firstVertexIndex + 5].texCoords = uvBottomLeft;
}
