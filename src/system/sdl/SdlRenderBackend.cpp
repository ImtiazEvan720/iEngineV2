#include "system/sdl/SdlRenderBackend.h"

#include "system/sdl/SdlWindowBackend.h"

#include <SDL3/SDL.h>
#ifdef IENGINE_USE_SDL_TTF
#include <SDL3_ttf/SDL_ttf.h>
#endif

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {
SDL_Texture* toSdlTexture(RenderTextureHandle texture) {
    return const_cast<SDL_Texture*>(static_cast<const SDL_Texture*>(texture));
}

float colorToFloat(std::uint8_t value) {
    return static_cast<float>(value) / 255.0f;
}

std::string makeFontKey(const std::string& fontPath, unsigned int characterSize) {
    return fontPath + "#" + std::to_string(characterSize);
}
}

SdlRenderBackend::SdlRenderBackend(SdlWindowBackend& windowBackend)
    : windowBackend(windowBackend) {
    initializeTtf();
}

SdlRenderBackend::~SdlRenderBackend() {
    clearFonts();

#ifdef IENGINE_USE_SDL_TTF
    if (ttfInitialized) {
        TTF_Quit();
    }
#endif
}

bool SdlRenderBackend::initializeTtf() {
#ifdef IENGINE_USE_SDL_TTF
    if (ttfInitialized) {
        return true;
    }

    ttfInitialized = TTF_Init();
    if (!ttfInitialized) {
        std::cerr << "Failed to initialize SDL_ttf: " << SDL_GetError() << std::endl;
    }

    return ttfInitialized;
#else
    return false;
#endif
}

TTF_Font* SdlRenderBackend::getFont(
    const std::string& fontPath,
    unsigned int characterSize
) {
#ifdef IENGINE_USE_SDL_TTF
    if (fontPath.empty() || characterSize == 0 || !initializeTtf()) {
        return nullptr;
    }

    const std::string fontKey = makeFontKey(fontPath, characterSize);
    const auto fontIterator = fonts.find(fontKey);
    if (fontIterator != fonts.end()) {
        return fontIterator->second;
    }

    if (failedFontKeys.find(fontKey) != failedFontKeys.end()) {
        return nullptr;
    }

    TTF_Font* font = TTF_OpenFont(fontPath.c_str(), static_cast<float>(characterSize));
    if (font == nullptr) {
        failedFontKeys.insert(fontKey);
        std::cerr << "Failed to load SDL_ttf font: " << fontPath
                  << " size=" << characterSize
                  << " error=" << SDL_GetError() << std::endl;
        return nullptr;
    }

    fonts.emplace(fontKey, font);
    return font;
#else
    (void)fontPath;
    (void)characterSize;
    return nullptr;
#endif
}

void SdlRenderBackend::clearFonts() {
#ifdef IENGINE_USE_SDL_TTF
    for (auto& fontEntry : fonts) {
        TTF_CloseFont(fontEntry.second);
    }
#endif

    fonts.clear();
    failedFontKeys.clear();
}

void SdlRenderBackend::drawTexture(
    RenderTextureHandle texture,
    const RenderRect& source,
    const RenderRect& destination,
    const RenderVector2& origin,
    float rotationDegrees
) {
    if (texture == nullptr || windowBackend.getRenderer() == nullptr) {
        return;
    }

    SDL_Texture* sdlTexture = toSdlTexture(texture);
    const float scaleX = source.width != 0.0f ? destination.width / source.width : 1.0f;
    const float scaleY = source.height != 0.0f ? destination.height / source.height : 1.0f;
    const SDL_FPoint scaledOrigin{
        origin.x * scaleX,
        origin.y * scaleY
    };

    const SDL_FRect sourceRect{
        source.x,
        source.y,
        source.width,
        source.height
    };
    const SDL_FRect destinationRect{
        destination.x - scaledOrigin.x,
        destination.y - scaledOrigin.y,
        destination.width,
        destination.height
    };

    SDL_RenderTextureRotated(
        windowBackend.getRenderer(),
        sdlTexture,
        &sourceRect,
        &destinationRect,
        static_cast<double>(rotationDegrees),
        &scaledOrigin,
        SDL_FLIP_NONE
    );
}

void SdlRenderBackend::drawGeometry(
    RenderTextureHandle texture,
    const std::vector<RenderVertex>& vertices
) {
    if (texture == nullptr || vertices.empty() || windowBackend.getRenderer() == nullptr) {
        return;
    }

    SDL_Texture* sdlTexture = toSdlTexture(texture);
    float textureWidth = 0.0f;
    float textureHeight = 0.0f;
    if (!SDL_GetTextureSize(sdlTexture, &textureWidth, &textureHeight)
        || textureWidth == 0.0f
        || textureHeight == 0.0f) {
        return;
    }

    std::vector<SDL_Vertex> sdlVertices;
    sdlVertices.reserve(vertices.size());

    for (const RenderVertex& vertex : vertices) {
        sdlVertices.push_back(SDL_Vertex{
            SDL_FPoint{vertex.x, vertex.y},
            SDL_FColor{
                colorToFloat(vertex.color.r),
                colorToFloat(vertex.color.g),
                colorToFloat(vertex.color.b),
                colorToFloat(vertex.color.a)
            },
            SDL_FPoint{vertex.u / textureWidth, vertex.v / textureHeight}
        });
    }

    SDL_RenderGeometry(
        windowBackend.getRenderer(),
        sdlTexture,
        sdlVertices.data(),
        static_cast<int>(sdlVertices.size()),
        nullptr,
        0
    );
}

void SdlRenderBackend::drawPoint(
    const RenderVector2& position,
    float radius,
    RenderColor color
) {
    SDL_Renderer* renderer = windowBackend.getRenderer();
    if (renderer == nullptr || radius <= 0.0f) {
        return;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    const int integerRadius = static_cast<int>(std::ceil(radius));
    for (int y = -integerRadius; y <= integerRadius; ++y) {
        const float yOffset = static_cast<float>(y);
        const float halfWidth = std::sqrt(std::max(0.0f, (radius * radius) - (yOffset * yOffset)));
        SDL_RenderLine(
            renderer,
            position.x - halfWidth,
            position.y + yOffset,
            position.x + halfWidth,
            position.y + yOffset
        );
    }
}

void SdlRenderBackend::drawText(
    const std::string& text,
    const std::string& fontPath,
    const RenderVector2& position,
    unsigned int characterSize,
    RenderColor color
) {
#ifdef IENGINE_USE_SDL_TTF
    SDL_Renderer* renderer = windowBackend.getRenderer();
    if (renderer == nullptr || text.empty() || characterSize == 0) {
        return;
    }

    TTF_Font* font = getFont(fontPath, characterSize);
    if (font == nullptr) {
        return;
    }

    const SDL_Color textColor{color.r, color.g, color.b, color.a};
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, text.c_str(), 0, textColor);
    if (textSurface == nullptr) {
        std::cerr << "Failed to render SDL_ttf text: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    const SDL_FRect destinationRect{
        position.x,
        position.y,
        static_cast<float>(textSurface->w),
        static_cast<float>(textSurface->h)
    };
    SDL_DestroySurface(textSurface);

    if (textTexture == nullptr) {
        std::cerr << "Failed to create SDL text texture: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_SetTextureBlendMode(textTexture, SDL_BLENDMODE_BLEND);
    SDL_RenderTexture(renderer, textTexture, nullptr, &destinationRect);
    SDL_DestroyTexture(textTexture);
#else
    (void)text;
    (void)fontPath;
    (void)position;
    (void)characterSize;
    (void)color;
#endif
}

void SdlRenderBackend::drawRect(
    const RenderRect& rect,
    RenderColor color
) {
    SDL_Renderer* renderer = windowBackend.getRenderer();
    if (renderer == nullptr || rect.width <= 0.0f || rect.height <= 0.0f) {
        return;
    }

    const SDL_FRect destinationRect{
        rect.x,
        rect.y,
        rect.width,
        rect.height
    };

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &destinationRect);
}

void SdlRenderBackend::clear(RenderColor color) {
    SDL_Renderer* renderer = windowBackend.getRenderer();
    if (renderer == nullptr) {
        return;
    }

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(renderer);
}

RenderTargetHandle SdlRenderBackend::createRenderTarget(int width, int height) {
    SDL_Renderer* renderer = windowBackend.getRenderer();
    if (renderer == nullptr || width <= 0 || height <= 0) {
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_TARGET,
        width,
        height
    );
    if (texture == nullptr) {
        return nullptr;
    }

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return texture;
}

void SdlRenderBackend::destroyRenderTarget(RenderTargetHandle target) {
    SDL_DestroyTexture(static_cast<SDL_Texture*>(target));
}

void SdlRenderBackend::beginRenderTarget(RenderTargetHandle target) {
    SDL_Renderer* renderer = windowBackend.getRenderer();
    if (renderer == nullptr) {
        return;
    }

    SDL_SetRenderTarget(renderer, static_cast<SDL_Texture*>(target));
}

void SdlRenderBackend::endRenderTarget() {
    SDL_Renderer* renderer = windowBackend.getRenderer();
    if (renderer == nullptr) {
        return;
    }

    SDL_SetRenderTarget(renderer, nullptr);
}

RenderTextureHandle SdlRenderBackend::getRenderTargetTexture(RenderTargetHandle target) {
    return static_cast<SDL_Texture*>(target);
}

ImTextureID SdlRenderBackend::getImGuiTextureId(RenderTextureHandle texture) {
    return reinterpret_cast<ImTextureID>(const_cast<void*>(texture));
}

bool SdlRenderBackend::readScreenPixel(int x, int y, RenderColor& outColor) {
    SDL_Surface* surface = SDL_RenderReadPixels(windowBackend.getRenderer(), nullptr);
    if (surface == nullptr) {
        return false;
    }

    if (x < 0 || y < 0 || x >= surface->w || y >= surface->h) {
        SDL_DestroySurface(surface);
        return false;
    }

    const std::uint8_t* pixels = static_cast<const std::uint8_t*>(surface->pixels);
    const std::uint8_t* pixel = pixels + (y * surface->pitch) + (x * 4);

    outColor = RenderColor{pixel[0], pixel[1], pixel[2], pixel[3]};

    SDL_DestroySurface(surface);
    return true;
}
