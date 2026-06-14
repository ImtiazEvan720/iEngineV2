#ifndef IENGINEV2_SFMLRENDERBACKEND_H
#define IENGINEV2_SFMLRENDERBACKEND_H

#include "system/IRenderBackend.h"

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

private:
    SfmlWindowBackend& windowBackend;
};

#endif
