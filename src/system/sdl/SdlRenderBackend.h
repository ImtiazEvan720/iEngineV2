#pragma once

#include "system/IRenderBackend.h"

class SdlWindowBackend;

class SdlRenderBackend : public IRenderBackend {
public:
    explicit SdlRenderBackend(SdlWindowBackend& windowBackend);

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

private:
    SdlWindowBackend& windowBackend;
};
