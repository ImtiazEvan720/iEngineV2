#ifndef IENGINEV2_IRENDERBACKEND_H
#define IENGINEV2_IRENDERBACKEND_H

#include <cstdint>
#include <vector>

struct RenderColor {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;
};

struct RenderRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct RenderVector2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct RenderVertex {
    float x = 0.0f;
    float y = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    RenderColor color{255, 255, 255, 255};
};

using RenderTextureHandle = const void*;

class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    virtual void drawTexture(
        RenderTextureHandle texture,
        const RenderRect& source,
        const RenderRect& destination,
        const RenderVector2& origin,
        float rotationDegrees
    ) = 0;

    virtual void drawGeometry(
        RenderTextureHandle texture,
        const std::vector<RenderVertex>& vertices
    ) = 0;
};

#endif
