#pragma once
#include "math/Vector2F.h"
#include "components/Component.h"

class RectTransformComponent: public Component {
public:
    RectTransformComponent(); 
    RectTransformComponent(const Vector2F& anchorPos, const Vector2F& sizeVal, const Vector2F& pivotVal, float rotationVal);

    ~RectTransformComponent() = default;
    std::unique_ptr<Component> clone() const override;

    void setAnchoredPosition(const Vector2F anchorPos);
    void setSize(const Vector2F sizeVal);
    void setPivot(const Vector2F pivotVal);
    void setRotation(float rotationVal);
    const Vector2F& getAnchoredPosition() const;
    const Vector2F& getSize() const;
    const Vector2F& getPivot() const;
    float getRotation() const;

private:
    Vector2F anchorPosition = Vector2F::zero();
    Vector2F size = Vector2F::zero();
    Vector2F pivot = Vector2F::zero();
    float rotation = 0.0f;
};
