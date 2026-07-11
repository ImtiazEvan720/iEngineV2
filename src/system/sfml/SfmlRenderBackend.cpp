#include "system/sfml/SfmlRenderBackend.h"

#include "system/sfml/SfmlWindowBackend.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/Image.hpp>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

SfmlRenderBackend::SfmlRenderBackend(SfmlWindowBackend& windowBackend)
    : windowBackend(windowBackend) {}

SfmlRenderBackend::~SfmlRenderBackend() = default;

sf::RenderTarget& SfmlRenderBackend::getCurrentTarget() {
    if (activeRenderTarget != nullptr) {
        return *activeRenderTarget;
    }

    return windowBackend.getWindow();
}

sf::Font* SfmlRenderBackend::getFont(const std::string& fontPath) {
    if (fontPath.empty()) {
        return nullptr;
    }

    const auto fontIterator = fonts.find(fontPath);
    if (fontIterator != fonts.end()) {
        return fontIterator->second.get();
    }

    if (failedFontPaths.find(fontPath) != failedFontPaths.end()) {
        return nullptr;
    }

    auto font = std::make_unique<sf::Font>();
    if (!font->openFromFile(fontPath)) {
        failedFontPaths.insert(fontPath);
        std::cerr << "Failed to load SFML font: " << fontPath << std::endl;
        return nullptr;
    }

    sf::Font* fontPointer = font.get();
    fonts.emplace(fontPath, std::move(font));
    return fontPointer;
}

void SfmlRenderBackend::drawTexture(
    RenderTextureHandle texture,
    const RenderRect& source,
    const RenderRect& destination,
    const RenderVector2& origin,
    float rotationDegrees
) {
    if (texture == nullptr) {
        return;
    }

    sf::RenderTarget& target = getCurrentTarget();
    const sf::Texture* sfmlTexture = static_cast<const sf::Texture*>(texture);
    sf::Sprite sprite(*sfmlTexture);
    sprite.setTextureRect(sf::IntRect(
        {static_cast<int>(source.x), static_cast<int>(source.y)},
        {static_cast<int>(source.width), static_cast<int>(source.height)}
    ));
    sprite.setOrigin({origin.x, origin.y});
    sprite.setPosition({destination.x, destination.y});
    sprite.setRotation(sf::degrees(rotationDegrees));

    if (source.width != 0.0f && source.height != 0.0f) {
        sprite.setScale({
            destination.width / source.width,
            destination.height / source.height
        });
    }

    sf::RenderStates states;
    states.blendMode = sf::BlendAlpha;
    target.draw(sprite, states);
}

void SfmlRenderBackend::drawGeometry(
    RenderTextureHandle texture,
    const std::vector<RenderVertex>& vertices
) {
    if (texture == nullptr || vertices.empty()) {
        return;
    }

    sf::RenderTarget& target = getCurrentTarget();
    std::vector<sf::Vertex> sfmlVertices;
    sfmlVertices.reserve(vertices.size());

    for (const RenderVertex& vertex : vertices) {
        sfmlVertices.push_back(sf::Vertex{
            {vertex.x, vertex.y},
            sf::Color(vertex.color.r, vertex.color.g, vertex.color.b, vertex.color.a),
            {vertex.u, vertex.v}
        });
    }

    sf::RenderStates states;
    states.blendMode = sf::BlendAlpha;
    states.texture = static_cast<const sf::Texture*>(texture);
    target.draw(sfmlVertices.data(), sfmlVertices.size(), sf::PrimitiveType::Triangles, states);
}

void SfmlRenderBackend::drawPoint(
    const RenderVector2& position,
    float radius,
    RenderColor color
) {
    if (radius <= 0.0f) {
        return;
    }

    sf::CircleShape point(radius);
    point.setOrigin({radius, radius});
    point.setPosition({position.x, position.y});
    point.setFillColor(sf::Color(color.r, color.g, color.b, color.a));
    sf::RenderTarget& target = getCurrentTarget();
    sf::RenderStates states;
    states.blendMode = sf::BlendAlpha;
    target.draw(point, states);
}

void SfmlRenderBackend::drawText(
    const std::string& text,
    const std::string& fontPath,
    const RenderVector2& position,
    const RenderVector2& origin,
    float rotationDegrees,
    unsigned int characterSize,
    RenderColor color
) {
    if (text.empty() || characterSize == 0) {
        return;
    }

    sf::Font* font = getFont(fontPath);
    if (font == nullptr) {
        return;
    }

    sf::Text drawableText(*font, text, characterSize);
    drawableText.setPosition({position.x, position.y});
    drawableText.setOrigin({origin.x, origin.y});
    drawableText.setRotation(sf::degrees(rotationDegrees));
    drawableText.setFillColor(sf::Color(color.r, color.g, color.b, color.a));
    sf::RenderTarget& target = getCurrentTarget();
    sf::RenderStates states;
    states.blendMode = sf::BlendAlpha;
    target.draw(drawableText, states);
}

RenderVector2 SfmlRenderBackend::measureText(
    const std::string& text,
    const std::string& fontPath,
    unsigned int characterSize
) {
    if (characterSize == 0) {
        return RenderVector2{};
    }

    sf::Font* font = getFont(fontPath);
    if (font == nullptr) {
        return RenderVector2{};
    }

    if (text.empty()) {
        return RenderVector2{0.0f, static_cast<float>(characterSize)};
    }

    sf::Text drawableText(*font, text, characterSize);
    const std::vector<sf::Text::ShapedGlyph>& shapedGlyphs = drawableText.getShapedGlyphs();

    float cursorX = 0.0f;
    float height = 0.0f;
    for (const sf::Text::ShapedGlyph& shapedGlyph : shapedGlyphs) {
        const sf::FloatRect& bounds = shapedGlyph.glyph.bounds;
        const float glyphLeft = shapedGlyph.position.x + bounds.position.x;
        const float glyphRight = glyphLeft + bounds.size.x;
        const float glyphTop = shapedGlyph.baseline + bounds.position.y;
        const float glyphBottom = glyphTop + bounds.size.y;

        cursorX = std::max(cursorX, glyphRight);
        height = std::max(height, glyphBottom);
    }

    const sf::FloatRect localBounds = drawableText.getLocalBounds();
    cursorX = std::max(cursorX, localBounds.position.x + localBounds.size.x);
    height = std::max(height, localBounds.position.y + localBounds.size.y);

    return RenderVector2{
        cursorX,
        height > 0.0f ? height : static_cast<float>(characterSize)
    };
}

void SfmlRenderBackend::drawRect(
    const RenderRect& rect,
    const RenderVector2& origin,
    float rotationDegrees,
    RenderColor color
) {
    if (rect.width <= 0.0f || rect.height <= 0.0f) {
        return;
    }

    sf::RectangleShape rectangle({rect.width, rect.height});
    rectangle.setPosition({rect.x, rect.y});
    rectangle.setOrigin({origin.x, origin.y});
    rectangle.setRotation(sf::degrees(rotationDegrees));
    rectangle.setFillColor(sf::Color(color.r, color.g, color.b, color.a));
    sf::RenderTarget& target = getCurrentTarget();
    sf::RenderStates states;
    states.blendMode = sf::BlendAlpha;
    target.draw(rectangle, states);
}

void SfmlRenderBackend::clear(RenderColor color) {
    getCurrentTarget().clear(sf::Color(color.r, color.g, color.b, color.a));
}

RenderTargetHandle SfmlRenderBackend::createRenderTarget(int width, int height) {
    if (width <= 0 || height <= 0) {
        return nullptr;
    }

    auto target = std::make_unique<sf::RenderTexture>();
    if (!target->resize({static_cast<unsigned int>(width), static_cast<unsigned int>(height)})) {
        return nullptr;
    }

    target->setSmooth(false);
    return target.release();
}

void SfmlRenderBackend::destroyRenderTarget(RenderTargetHandle target) {
    sf::RenderTexture* sfmlRenderTarget = static_cast<sf::RenderTexture*>(target);
    if (activeRenderTarget == sfmlRenderTarget) {
        activeRenderTarget = nullptr;
    }

    delete sfmlRenderTarget;
}

void SfmlRenderBackend::beginRenderTarget(RenderTargetHandle target) {
    activeRenderTarget = static_cast<sf::RenderTexture*>(target);
    if (activeRenderTarget != nullptr) {
        activeRenderTarget->resetGLStates();
        activeRenderTarget->setView(activeRenderTarget->getDefaultView());
    }
}

void SfmlRenderBackend::endRenderTarget() {
    if (activeRenderTarget != nullptr) {
        activeRenderTarget->display();
        activeRenderTarget = nullptr;
    }
}

RenderTextureHandle SfmlRenderBackend::getRenderTargetTexture(RenderTargetHandle target) {
    sf::RenderTexture* sfmlRenderTarget = static_cast<sf::RenderTexture*>(target);
    if (sfmlRenderTarget == nullptr) {
        return nullptr;
    }

    return &sfmlRenderTarget->getTexture();
}

ImTextureID SfmlRenderBackend::getImGuiTextureId(RenderTextureHandle texture) {
    ImTextureID textureId{};
    if (texture == nullptr) {
        return textureId;
    }

    const sf::Texture* sfmlTexture = static_cast<const sf::Texture*>(texture);
    const auto nativeHandle = sfmlTexture->getNativeHandle();
    static_assert(sizeof(nativeHandle) <= sizeof(ImTextureID),
                  "ImTextureID is not large enough for an SFML texture handle.");
    std::memcpy(&textureId, &nativeHandle, sizeof(nativeHandle));
    return textureId;
}

bool SfmlRenderBackend::readScreenPixel(int x, int y, RenderColor& outColor) {
    if (activeRenderTarget != nullptr) {
        const sf::Vector2u size = activeRenderTarget->getSize();

        if (x < 0 || y < 0 ||
            x >= static_cast<int>(size.x) ||
            y >= static_cast<int>(size.y)) {
            return false;
        }

        activeRenderTarget->display();

        const sf::Image screenshot = activeRenderTarget->getTexture().copyToImage();
        const sf::Color pixelColor = screenshot.getPixel({
            static_cast<unsigned int>(x),
            static_cast<unsigned int>(y)
        });

        outColor = RenderColor{pixelColor.r, pixelColor.g, pixelColor.b, pixelColor.a};
        return true;
    }

    sf::RenderWindow& window = windowBackend.getWindow();
    const sf::Vector2u size = window.getSize();

    if (x < 0 || y < 0 ||
        x >= static_cast<int>(size.x) ||
        y >= static_cast<int>(size.y)) {
        return false;
    }

    sf::Texture screenshotTexture;
    if (!screenshotTexture.resize(size)) {
        return false;
    }

    screenshotTexture.update(window);

    const sf::Image screenshot = screenshotTexture.copyToImage();
    const sf::Color pixelColor = screenshot.getPixel({
        static_cast<unsigned int>(x),
        static_cast<unsigned int>(y)
    });

    outColor = RenderColor{pixelColor.r, pixelColor.g, pixelColor.b, pixelColor.a};
    return true;
}
