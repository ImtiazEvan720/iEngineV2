#pragma once

#include "imgui.h"

#include <cstdint>
#include <string>
#include <vector>

struct RenderColor
{
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;
};

struct RenderRect
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct RenderVector2
{
    float x = 0.0f;
    float y = 0.0f;
};

struct RenderVertex
{
    float x = 0.0f;
    float y = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    RenderColor color{255, 255, 255, 255};
};

using RenderTextureHandle = const void *;
using RenderTargetHandle = void *;

class IRenderBackend
{
public:
    virtual ~IRenderBackend() = default;

    virtual void drawTexture(
        RenderTextureHandle texture,
        const RenderRect &source,
        const RenderRect &destination,
        const RenderVector2 &origin,
        float rotationDegrees) = 0;

    virtual void drawGeometry(
        RenderTextureHandle texture,
        const std::vector<RenderVertex> &vertices) = 0;

    virtual void drawPoint(
        const RenderVector2 &position,
        float radius,
        RenderColor color) = 0;

    virtual void drawText(
        const std::string &text,
        const std::string &fontPath,
        const RenderVector2 &position,
        unsigned int characterSize,
        RenderColor color) = 0;

    virtual void drawRect(
        const RenderRect &rect,
        RenderColor color
    ) = 0;

    virtual void clear(RenderColor color) = 0;
    virtual RenderTargetHandle createRenderTarget(int width, int height) = 0;
    virtual void destroyRenderTarget(RenderTargetHandle target) = 0;
    virtual void beginRenderTarget(RenderTargetHandle target) = 0;
    virtual void endRenderTarget() = 0;
    virtual RenderTextureHandle getRenderTargetTexture(RenderTargetHandle target) = 0;
    virtual ImTextureID getImGuiTextureId(RenderTextureHandle texture) = 0;
    virtual bool readScreenPixel(int x, int y, RenderColor& outColor) = 0;
};
