#include "system/sdl/SdlRenderBackend.h"

#include "system/sdl/SdlWindowBackend.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {
SDL_Texture* toSdlTexture(RenderTextureHandle texture) {
    return const_cast<SDL_Texture*>(static_cast<const SDL_Texture*>(texture));
}

float colorToFloat(std::uint8_t value) {
    return static_cast<float>(value) / 255.0f;
}
}

SdlRenderBackend::SdlRenderBackend(SdlWindowBackend& windowBackend)
    : windowBackend(windowBackend) {}

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
