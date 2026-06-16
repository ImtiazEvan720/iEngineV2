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

private:
    std::unique_ptr<sf::Texture> texture;
};

#endif
