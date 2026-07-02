#pragma once

#include "system/IRenderBackend.h"

#include <string>
#include <unordered_map>
#include <unordered_set>

struct TTF_Font;

class SdlWindowBackend;

class SdlRenderBackend : public IRenderBackend {
public:
    explicit SdlRenderBackend(SdlWindowBackend& windowBackend);
    ~SdlRenderBackend() override;

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
    void drawText(
        const std::string& text,
        const std::string& fontPath,
        const RenderVector2& position,
        unsigned int characterSize,
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
    bool initializeTtf();
    TTF_Font* getFont(const std::string& fontPath, unsigned int characterSize);
    void clearFonts();

    SdlWindowBackend& windowBackend;
    std::unordered_map<std::string, TTF_Font*> fonts;
    std::unordered_set<std::string> failedFontKeys;
    bool ttfInitialized = false;
};
