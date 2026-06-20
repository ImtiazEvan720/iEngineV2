#ifndef IENGINEV2_RENDERER_H
#define IENGINEV2_RENDERER_H

#include "misc/Camera2D.h"
#include "system/TileLayerRenderer.h"

class IWindowBackend;
class LevelAsset;

class Renderer {
public:
    static Renderer& getInstance();
    void setRenderBackend(IRenderBackend* backend);
    void setWindowBackend(IWindowBackend* backend);
    void setRenderScale(float scale);
    float getRenderScale() const;
    Camera2D& getCamera();
    const Camera2D& getCamera() const;
    RenderRect getViewport() const;
    float consumePendingPinchZoomFactor();
    bool buildTileLayerBatches(LevelAsset& levelAsset);
    void clearTileLayerBatches();
    void update(float deltaTime);
    void setEditorViewportActivity(bool dragDropActive, bool gridVisible, bool cameraActive);
    void updateEditorOnly(float deltaTime);
    bool shouldRenderEditorViewport() const;
    void render();

    Renderer(const Renderer& other) = delete;
    Renderer& operator=(const Renderer& other) = delete;
    Renderer(Renderer&& other) = delete;
    Renderer& operator=(Renderer&& other) = delete;

private:
    Renderer() = default;
    ~Renderer() = default;

    IRenderBackend* renderBackend = nullptr;
    IWindowBackend* windowBackend = nullptr;
    float renderScale = 4.0f;
    bool editorDragDropActive = false;
    bool editorGridVisible = false;
    bool editorCameraActive = false;
    bool editorViewportRenderRequested = false;
    Camera2D camera;
    TileLayerRenderer tileLayerRenderer;
};

#endif
