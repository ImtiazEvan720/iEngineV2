#ifndef IENGINEV2_TILELAYERRENDERER_H
#define IENGINEV2_TILELAYERRENDERER_H

#include "misc/LevelAsset.h"

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace sf {
class RenderWindow;
class Texture;
}

class TileLayerRenderer {
public:
    bool buildFromLevelAsset(LevelAsset& levelAsset, float renderScale);
    void clear();
    void update(float deltaTime);
    void render(sf::RenderWindow& window) const;

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
        sf::Texture* texture = nullptr;
        sf::VertexArray vertices;
        std::vector<AnimatedTileRef> animatedTiles;
    };

    struct RenderLayer {
        std::string groupName;
        std::string layerName;
        std::vector<TileChunk> chunks;
    };

    TileChunk& getOrCreateChunk(RenderLayer& renderLayer, int chunkX, int chunkY, sf::Texture* texture);
    std::size_t appendTileQuad(
        sf::VertexArray& vertices,
        float destinationX,
        float destinationY,
        float destinationWidth,
        float destinationHeight,
        const sf::IntRect& sourceRect
    ) const;
    void updateTileTexCoords(sf::VertexArray& vertices, std::size_t firstVertexIndex, const sf::IntRect& sourceRect) const;

    std::vector<RenderLayer> layers;
};

#endif
