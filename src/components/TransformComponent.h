#pragma once

#include "components/Component.h"
#include "math/Vector2F.h"

class TransformComponent : public Component {
public:
    TransformComponent();
    TransformComponent(const Vector2F& position, float rotation);

    const Vector2F& getPosition() const;
    float getRotation() const;
    Vector2F getWorldPosition() const;
    float getWorldRotation() const;
    TransformComponent* getParent() const;

    void setPosition(const Vector2F& position);
    void setRotation(float rotation);
    void setParent(TransformComponent* parent);

private:
    Vector2F position;
    float rotation;
    TransformComponent* parent = nullptr;
};
