#pragma once

#include "system/IRenderBackend.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace sf {
class Font;
class RenderTarget;
class RenderTexture;
}

class SfmlWindowBackend;

class SfmlRenderBackend : public IRenderBackend {
public:
    explicit SfmlRenderBackend(SfmlWindowBackend& windowBackend);
    ~SfmlRenderBackend() override;

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
        const RenderVector2& origin,
        float rotationDegrees,
        unsigned int characterSize,
        RenderColor color
    ) override;
    RenderVector2 measureText(
        const std::string& text,
        const std::string& fontPath,
        unsigned int characterSize
    ) override;
    void drawRect(
        const RenderRect& rect,
        const RenderVector2& origin,
        float rotationDegrees,
        RenderColor color
    ) override;

    void clear(RenderColor color) override;
    RenderTargetHandle createRenderTarget(int width, int height) override;
    void destroyRenderTarget(RenderTargetHandle target) override;
    void beginRenderTarget(RenderTargetHandle target) override;
    void endRenderTarget() override;
    RenderTextureHandle getRenderTargetTexture(RenderTargetHandle target) override;
    ImTextureID getImGuiTextureId(RenderTextureHandle texture) override;
    bool readScreenPixel(int x, int y, RenderColor& outColor) override;

private:
    sf::RenderTarget& getCurrentTarget();
    sf::Font* getFont(const std::string& fontPath);

    SfmlWindowBackend& windowBackend;
    sf::RenderTexture* activeRenderTarget = nullptr;
    std::unordered_map<std::string, std::unique_ptr<sf::Font>> fonts;
    std::unordered_set<std::string> failedFontPaths;
};
