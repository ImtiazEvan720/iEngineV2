#pragma once

#include "system/IRenderBackend.h"

namespace sf {
class RenderTarget;
class RenderTexture;
}

class SfmlWindowBackend;

class SfmlRenderBackend : public IRenderBackend {
public:
    explicit SfmlRenderBackend(SfmlWindowBackend& windowBackend);

    void drawTexture(
        RenderTextureHandle texture,
        const RenderRect& source,
        const RenderRect& destination,
        const RenderVector2& origin,
        float rotationDegrees
    ) override;
    void drawGeometry(
        RenderTextureHandle texture,
        const std::vector<RenderVertex>& vertices
    ) override;
    void drawPoint(
        const RenderVector2& position,
        float radius,
        RenderColor color
    ) override;

    void clear(RenderColor color) override;
    RenderTargetHandle createRenderTarget(int width, int height) override;
    void destroyRenderTarget(RenderTargetHandle target) override;
    void beginRenderTarget(RenderTargetHandle target) override;
    void endRenderTarget() override;
    RenderTextureHandle getRenderTargetTexture(RenderTargetHandle target) override;
    ImTextureID getImGuiTextureId(RenderTextureHandle texture) override;

private:
    sf::RenderTarget& getCurrentTarget();

    SfmlWindowBackend& windowBackend;
    sf::RenderTexture* activeRenderTarget = nullptr;
};
