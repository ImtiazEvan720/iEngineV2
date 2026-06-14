#include "system/sfml/SfmlRenderBackend.h"

#include "system/sfml/SfmlWindowBackend.h"

#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Vertex.hpp>

#include <vector>

SfmlRenderBackend::SfmlRenderBackend(SfmlWindowBackend& windowBackend)
    : windowBackend(windowBackend) {}

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

    sf::RenderWindow& window = windowBackend.getWindow();
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

    window.draw(sprite);
}

void SfmlRenderBackend::drawGeometry(
    RenderTextureHandle texture,
    const std::vector<RenderVertex>& vertices
) {
    if (texture == nullptr || vertices.empty()) {
        return;
    }

    sf::RenderWindow& window = windowBackend.getWindow();
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
    window.draw(sfmlVertices.data(), sfmlVertices.size(), sf::PrimitiveType::Triangles, states);
}
