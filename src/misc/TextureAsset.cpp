#include "misc/TextureAsset.h"

#include "misc/ITextureResource.h"
#include "misc/SdlTextureResource.h"

#ifndef __EMSCRIPTEN__
#include "misc/SfmlTextureResource.h"
#endif

#include <iostream>
#include <memory>
#include <utility>

namespace {
#ifdef __EMSCRIPTEN__
TextureRenderBackend activeBackend = TextureRenderBackend::Sdl;
#else
TextureRenderBackend activeBackend = TextureRenderBackend::Sfml;
#endif
void* activeNativeContext = nullptr;
}

TextureAsset::TextureAsset(std::string name, std::string path)
    : Asset(std::move(name), std::move(path), Type::Texture) {}

TextureAsset::~TextureAsset() = default;

void TextureAsset::setTextureRenderBackend(TextureRenderBackend backend, void* nativeContext) {
    activeBackend = backend;
    activeNativeContext = nativeContext;
}

bool TextureAsset::load() {
    if (activeBackend == TextureRenderBackend::Sdl) {
        textureResource = std::make_unique<SdlTextureResource>(
            static_cast<SDL_Renderer*>(activeNativeContext)
        );
    }
#ifndef __EMSCRIPTEN__
    else {
        textureResource = std::make_unique<SfmlTextureResource>();
    }
#else
    else {
        std::cerr << "SFML texture loading is not available in web builds: "
                  << getPath() << std::endl;
        return false;
    }
#endif

    setLoaded(textureResource->loadFromFile(getPath()));
    if (!isLoaded()) {
        std::cerr << "Failed to load " << typeToString(getType()) << ": " << getPath() << std::endl;
        textureResource.reset();
    }

    return isLoaded();
}

RenderTextureHandle TextureAsset::getTextureHandle() const {
    if (textureResource == nullptr) {
        return nullptr;
    }

    return textureResource->getHandle();
}
