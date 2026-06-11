#ifndef IENGINEV2_SPRITE_H
#define IENGINEV2_SPRITE_H

#include "math/Vector2F.h"

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Texture.hpp>

class Sprite {
public:
    Sprite(sf::Texture* texture, const sf::FloatRect& sourceRect);

    sf::Texture* getTexture() const;
    const sf::FloatRect& getSourceRect() const;
    const Vector2F& getOrigin() const;
    const Vector2F& getSize() const;

    void setTexture(sf::Texture* texture);
    void setSourceRect(const sf::FloatRect& sourceRect);
    void setOrigin(const Vector2F& origin);
    void setSize(const Vector2F& size);

private:
    sf::Texture* texture;
    sf::FloatRect sourceRect;
    Vector2F origin;
    Vector2F size;
};

#endif
