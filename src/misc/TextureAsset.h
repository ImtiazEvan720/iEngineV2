#ifndef IENGINEV2_TEXTUREASSET_H
#define IENGINEV2_TEXTUREASSET_H

#include "misc/Asset.h"
#include "system/IRenderBackend.h"

#include "imgui.h"

#include <memory>
#include <string>

struct SDL_Renderer;
class ITextureResource;

enum class TextureRenderBackend {
    Sfml,
    Sdl
};

class TextureAsset : public Asset {
public:
    TextureAsset(std::string name, std::string path);
    ~TextureAsset() override;

    static void setTextureRenderBackend(TextureRenderBackend backend, void* nativeContext);

    bool load() override;

    RenderTextureHandle getTextureHandle() const;
    ImTextureID getImGuiTextureId() const;
    int getWidth() const;
    int getHeight() const;

private:
    std::unique_ptr<ITextureResource> textureResource;
};

#endif
