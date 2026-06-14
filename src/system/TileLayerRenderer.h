#ifndef IENGINEV2_TILELAYERRENDERER_H
#define IENGINEV2_TILELAYERRENDERER_H

#include "misc/LevelAsset.h"
#include "system/IRenderBackend.h"

#include <cstddef>
#include <string>
#include <vector>

class TileLayerRenderer {
public:
    bool buildFromLevelAsset(LevelAsset& levelAsset, float renderScale);
    void clear();
    void update(float deltaTime);
    void render(IRenderBackend& renderBackend, const RenderRect& viewport) const;

private:
    static constexpr int chunkSize = 16;

    struct AnimatedTileRef {
        std::size_t firstVertexIndex = 0;
        const TilesetInfo* tileset = nullptr;
        const TileAnimation* animation = nullptr;
        float elapsedTime = 0.0f;
        std::size_t currentFrameIndex = 0;
    };

    struct TileChunk {
        int chunkX = 0;
        int chunkY = 0;
        RenderRect bounds;
        RenderTextureHandle texture = nullptr;
        std::vector<RenderVertex> vertices;
        std::vector<AnimatedTileRef> animatedTiles;
    };

    struct RenderLayer {
        std::string groupName;
        std::string layerName;
        std::vector<TileChunk> chunks;
    };

    TileChunk& getOrCreateChunk(
        RenderLayer& renderLayer,
        int chunkX,
        int chunkY,
        RenderTextureHandle texture,
        const RenderRect& bounds
    );
    std::size_t appendTileQuad(
        std::vector<RenderVertex>& vertices,
        float destinationX,
        float destinationY,
        float destinationWidth,
        float destinationHeight,
        const RenderRect& sourceRect
    ) const;
    void updateTileTexCoords(std::vector<RenderVertex>& vertices, std::size_t firstVertexIndex, const RenderRect& sourceRect) const;

    std::vector<RenderLayer> layers;
};

#endif
