#ifndef IENGINEV2_SFMLTEXTURERESOURCE_H
#define IENGINEV2_SFMLTEXTURERESOURCE_H

#include "misc/ITextureResource.h"

#include <SFML/Graphics/Texture.hpp>

#include <memory>

class SfmlTextureResource : public ITextureResource {
public:
    SfmlTextureResource() = default;
    ~SfmlTextureResource() override;

    bool loadFromFile(const std::string& path) override;
    RenderTextureHandle getHandle() const override;
    ImTextureID getImGuiTextureId() const override;
    int getWidth() const override;
    int getHeight() const override;

private:
    std::unique_ptr<sf::Texture> texture;
    int width = 0;
    int height = 0;
};

#endif
