#pragma once

#include "misc/Camera2D.h"
#include "system/IRenderBackend.h"
#include "system/TileLayerRenderer.h"

#include <vector>

class IWindowBackend;
class LevelAsset;
class Entity;
class PlayerCameraComponent;
class TransformComponent;

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
    bool isEntityInViewport(const Entity& entity) const;
    void debugDrawPoint(
        const Vector2F& worldPosition,
        float radius,
        RenderColor color,
        float lifetimeSeconds = 0.0f
    );
    void clearDebugDraw();
    void render();
    ImTextureID renderCameraPreview(
        const TransformComponent& transform,
        const PlayerCameraComponent& camera,
        int width,
        int height
    );

    bool pickScreenColor(int x, int y, RenderColor& outColor);

    Renderer(const Renderer& other) = delete;
    Renderer& operator=(const Renderer& other) = delete;
    Renderer(Renderer&& other) = delete;
    Renderer& operator=(Renderer&& other) = delete;

private:
    struct DebugPoint {
        Vector2F worldPosition;
        float radius = 4.0f;
        RenderColor color{255, 0, 0, 255};
        float remainingSeconds = 0.0f;
        bool oneFrame = true;
    };

    Renderer() = default;
    ~Renderer() = default;

    void applyMainCamera(const RenderRect& viewport);
    void ensureCameraPreviewTarget(int width, int height);
    Camera2D buildCameraFromPlayerCamera(
        const TransformComponent& transform,
        const PlayerCameraComponent& cameraComponent,
        const RenderRect& viewport
    ) const;
    void renderWorld(const Camera2D& renderCamera, const RenderRect& viewport);
    void renderDebugPoints(const Camera2D& renderCamera, const RenderRect& viewport);
    void updateDebugPoints();
    void renderUI();

    IRenderBackend* renderBackend = nullptr;
    IWindowBackend* windowBackend = nullptr;
    RenderTargetHandle cameraPreviewTarget = nullptr;
    int cameraPreviewWidth = 0;
    int cameraPreviewHeight = 0;
    float renderScale = 4.0f;
    bool editorDragDropActive = false;
    bool editorGridVisible = false;
    bool editorCameraActive = false;
    bool editorViewportRenderRequested = false;
    float debugDrawDeltaTime = 0.0f;
    Camera2D camera;
    TileLayerRenderer tileLayerRenderer;
    std::vector<DebugPoint> debugPoints;
};
