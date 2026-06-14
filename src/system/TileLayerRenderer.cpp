#include "system/TileLayerRenderer.h"

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

RenderRect getTileSourceRect(const TilesetInfo& tileset, int localTileId) {
    const int sourceX = (localTileId % tileset.columns) * tileset.tileWidth;
    const int sourceY = (localTileId / tileset.columns) * tileset.tileHeight;

    return RenderRect{
        static_cast<float>(sourceX),
        static_cast<float>(sourceY),
        static_cast<float>(tileset.tileWidth),
        static_cast<float>(tileset.tileHeight)
    };
}

bool intersects(const RenderRect& left, const RenderRect& right) {
    return left.x < right.x + right.width
        && left.x + left.width > right.x
        && left.y < right.y + right.height
        && left.y + left.height > right.y;
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
    const float scaledChunkWidth = scaledTileWidth * static_cast<float>(chunkSize);
    const float scaledChunkHeight = scaledTileHeight * static_cast<float>(chunkSize);

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
                        || tileset->textureAsset->getTextureHandle() == nullptr) {
                        continue;
                    }

                    const int localTileId = gid - tileset->firstGid;
                    const int chunkX = x / chunkSize;
                    const int chunkY = y / chunkSize;
                    const RenderRect chunkBounds{
                        static_cast<float>(chunkX) * scaledChunkWidth,
                        static_cast<float>(chunkY) * scaledChunkHeight,
                        scaledChunkWidth,
                        scaledChunkHeight
                    };
                    TileChunk& chunk = getOrCreateChunk(
                        renderLayer,
                        chunkX,
                        chunkY,
                        tileset->textureAsset->getTextureHandle(),
                        chunkBounds
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

void TileLayerRenderer::render(IRenderBackend& renderBackend, const RenderRect& viewport) const {
    for (const RenderLayer& layer : layers) {
        for (const TileChunk& chunk : layer.chunks) {
            if (chunk.texture == nullptr || chunk.vertices.empty()) {
                continue;
            }

            if (!intersects(chunk.bounds, viewport)) {
                continue;
            }

            renderBackend.drawGeometry(chunk.texture, chunk.vertices);
        }
    }
}

TileLayerRenderer::TileChunk& TileLayerRenderer::getOrCreateChunk(
    RenderLayer& renderLayer,
    int chunkX,
    int chunkY,
    RenderTextureHandle texture,
    const RenderRect& bounds
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
    chunk.bounds = bounds;
    chunk.texture = texture;
    return chunk;
}

std::size_t TileLayerRenderer::appendTileQuad(
    std::vector<RenderVertex>& vertices,
    float destinationX,
    float destinationY,
    float destinationWidth,
    float destinationHeight,
    const RenderRect& sourceRect
) const {
    const float left = destinationX;
    const float top = destinationY;
    const float right = destinationX + destinationWidth;
    const float bottom = destinationY + destinationHeight;

    const float uvLeft = sourceRect.x;
    const float uvTop = sourceRect.y;
    const float uvRight = sourceRect.x + sourceRect.width;
    const float uvBottom = sourceRect.y + sourceRect.height;

    const std::size_t firstVertexIndex = vertices.size();

    vertices.push_back(RenderVertex{left, top, uvLeft, uvTop});
    vertices.push_back(RenderVertex{right, top, uvRight, uvTop});
    vertices.push_back(RenderVertex{right, bottom, uvRight, uvBottom});

    vertices.push_back(RenderVertex{left, top, uvLeft, uvTop});
    vertices.push_back(RenderVertex{right, bottom, uvRight, uvBottom});
    vertices.push_back(RenderVertex{left, bottom, uvLeft, uvBottom});

    return firstVertexIndex;
}

void TileLayerRenderer::updateTileTexCoords(
    std::vector<RenderVertex>& vertices,
    std::size_t firstVertexIndex,
    const RenderRect& sourceRect
) const {
    if (firstVertexIndex + 5 >= vertices.size()) {
        return;
    }

    const float uvLeft = sourceRect.x;
    const float uvTop = sourceRect.y;
    const float uvRight = sourceRect.x + sourceRect.width;
    const float uvBottom = sourceRect.y + sourceRect.height;

    vertices[firstVertexIndex + 0].u = uvLeft;
    vertices[firstVertexIndex + 0].v = uvTop;
    vertices[firstVertexIndex + 1].u = uvRight;
    vertices[firstVertexIndex + 1].v = uvTop;
    vertices[firstVertexIndex + 2].u = uvRight;
    vertices[firstVertexIndex + 2].v = uvBottom;

    vertices[firstVertexIndex + 3].u = uvLeft;
    vertices[firstVertexIndex + 3].v = uvTop;
    vertices[firstVertexIndex + 4].u = uvRight;
    vertices[firstVertexIndex + 4].v = uvBottom;
    vertices[firstVertexIndex + 5].u = uvLeft;
    vertices[firstVertexIndex + 5].v = uvBottom;
}
