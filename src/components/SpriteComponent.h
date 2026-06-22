#pragma once

#include "components/Component.h"
#include "misc/Sprite.h"

class SpriteComponent : public Component {
public:
    explicit SpriteComponent(const Sprite& sprite);

    Sprite& getSprite();
    const Sprite& getSprite() const;

    void setSprite(const Sprite& sprite);

private:
    Sprite sprite;
};
