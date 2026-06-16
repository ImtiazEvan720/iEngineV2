#include "misc/SdlTextureResource.h"

#include <SDL3/SDL.h>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstdint>
#include <iostream>

void SdlTextureDeleter::operator()(SDL_Texture* texture) const {
    if (texture != nullptr) {
        SDL_DestroyTexture(texture);
    }
}

SdlTextureResource::SdlTextureResource(SDL_Renderer* renderer)
    : renderer(renderer) {}

bool SdlTextureResource::loadFromFile(const std::string& path) {
    if (renderer == nullptr) {
        std::cerr << "Failed to load SDL texture without a renderer: " << path << std::endl;
        return false;
    }

    int width = 0;
    int height = 0;
    int channelCount = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channelCount, 4);
    if (pixels == nullptr || width <= 0 || height <= 0) {
        std::cerr << "Failed to load texture image: " << path
                  << " (" << stbi_failure_reason() << ")" << std::endl;
        return false;
    }

    for (int pixelIndex = 0; pixelIndex < width * height; ++pixelIndex) {
        std::uint8_t* pixel = pixels + pixelIndex * 4;
        if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 1) {
            pixel[3] = 0;
        }
    }

    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        width,
        height,
        SDL_PIXELFORMAT_RGBA32,
        pixels,
        width * 4
    );

    if (surface == nullptr) {
        std::cerr << "Failed to create SDL surface for texture " << path
                  << ": " << SDL_GetError() << std::endl;
        stbi_image_free(pixels);
        return false;
    }

    texture.reset(SDL_CreateTextureFromSurface(renderer, surface));
    SDL_DestroySurface(surface);
    stbi_image_free(pixels);

    if (texture == nullptr) {
        std::cerr << "Failed to create SDL texture for " << path
                  << ": " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND);
    return true;
}

RenderTextureHandle SdlTextureResource::getHandle() const {
    return texture.get();
}

ImTextureID SdlTextureResource::getImGuiTextureId() const {
    return reinterpret_cast<ImTextureID>(texture.get());
}
