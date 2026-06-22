#pragma once

#include "misc/ITextureResource.h"

#include <memory>

struct SDL_Renderer;
struct SDL_Texture;

struct SdlTextureDeleter {
    void operator()(SDL_Texture* texture) const;
};

class SdlTextureResource : public ITextureResource {
public:
    explicit SdlTextureResource(SDL_Renderer* renderer);
    ~SdlTextureResource() override = default;

    bool loadFromFile(const std::string& path) override;
    RenderTextureHandle getHandle() const override;
    ImTextureID getImGuiTextureId() const override;
    int getWidth() const override;
    int getHeight() const override;

private:
    SDL_Renderer* renderer = nullptr;
    std::unique_ptr<SDL_Texture, SdlTextureDeleter> texture;
    int width = 0;
    int height = 0;
};
