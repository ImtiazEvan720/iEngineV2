#ifndef IENGINEV2_RENDERER_H
#define IENGINEV2_RENDERER_H

#include <SFML/Graphics/RenderTexture.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace sf {
class RenderWindow;
class Texture;
}

class LevelAsset;

class Renderer {
public:
    static Renderer& getInstance();
    void setWindow(sf::RenderWindow* renderWindow);
    void setRenderScale(float scale);
    float getRenderScale() const;
    bool buildTileLayerBatches(LevelAsset& levelAsset);
    void clearTileLayerBatches();
    void update(float deltaTime);
    void render();

    Renderer(const Renderer& other) = delete;
    Renderer& operator=(const Renderer& other) = delete;
    Renderer(Renderer&& other) = delete;
    Renderer& operator=(Renderer&& other) = delete;

private:
    Renderer() = default;
    ~Renderer() = default;

    struct TileDrawInfo {
        sf::Texture* texture = nullptr;
        int sourceX = 0;
        int sourceY = 0;
        int sourceWidth = 0;
        int sourceHeight = 0;
        float destinationX = 0.0f;
        float destinationY = 0.0f;
        float destinationWidth = 0.0f;
        float destinationHeight = 0.0f;
    };

    struct AnimatedTileFrame {
        TileDrawInfo drawInfo;
        float durationSeconds = 0.0f;
    };

    struct AnimatedTileDrawInfo {
        std::vector<AnimatedTileFrame> frames;
        std::size_t currentFrameIndex = 0;
        float elapsedTime = 0.0f;
    };

    struct TileLayerBatch {
        std::string groupName;
        std::string layerName;
        sf::RenderTexture renderTexture;
        std::vector<TileDrawInfo> staticTiles;
        std::vector<AnimatedTileDrawInfo> animatedTiles;
        bool dirty = true;
    };

    void rebuildTileLayerBatch(TileLayerBatch& batch);

    sf::RenderWindow* window = nullptr;
    float renderScale = 4.0f;
    std::vector<TileLayerBatch> tileLayerBatches;
};

#endif
