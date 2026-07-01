#include "system/sfml/SfmlRenderBackend.h"

#include "system/sfml/SfmlWindowBackend.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Vertex.hpp>

#include <cstring>
#include <memory>
#include <vector>

SfmlRenderBackend::SfmlRenderBackend(SfmlWindowBackend& windowBackend)
    : windowBackend(windowBackend) {}

sf::RenderTarget& SfmlRenderBackend::getCurrentTarget() {
    if (activeRenderTarget != nullptr) {
        return *activeRenderTarget;
    }

    return windowBackend.getWindow();
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

    target.draw(sprite);
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
    getCurrentTarget().draw(point);
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
