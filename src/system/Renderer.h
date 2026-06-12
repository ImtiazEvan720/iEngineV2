#ifndef IENGINEV2_RENDERER_H
#define IENGINEV2_RENDERER_H

#include "system/TileLayerRenderer.h"

namespace sf {
class RenderWindow;
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

    sf::RenderWindow* window = nullptr;
    float renderScale = 4.0f;
    TileLayerRenderer tileLayerRenderer;
};

#endif
