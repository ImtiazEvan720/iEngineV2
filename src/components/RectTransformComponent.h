#pragma once
#include "math/Vector2F.h"
#include "components/Component.h"


enum class AnchorAlignment
{
    LeftTop,
    LeftCenter,
    LeftBottom,
    CenterTop,
    Center,
    CenterBottom,
    RightTop,
    RighTop = RightTop,
    RightCenter,
    RightBottom
};

class RectTransformComponent: public Component {
public:
    RectTransformComponent(); 
    RectTransformComponent(
        const Vector2F& anchorPos,
        const Vector2F& sizeVal,
        const Vector2F& pivotVal,
        float rotationVal,
        AnchorAlignment alignmentVal = AnchorAlignment::LeftTop
    );

    ~RectTransformComponent() = default;
    std::unique_ptr<Component> clone() const override;

    void setAnchoredPosition(const Vector2F& anchorPos);
    void setAnchorAlignment(const AnchorAlignment alignmentValue);
    void setWorldPosition(const Vector2F& worldPosition);
    void setSize(const Vector2F& sizeVal);
    void setPivot(const Vector2F& pivotVal);
    void setRotation(float rotationVal);
    void setWorldRotation(float worldRotation);
    void setParent(RectTransformComponent* parent);

    const Vector2F& getAnchoredPosition() const;
    AnchorAlignment getAnchorAlignment() const;
    Vector2F getWorldPosition() const;
    const Vector2F& getSize() const;
    const Vector2F& getPivot() const;
    float getRotation() const;
    float getWorldRotation() const;
    RectTransformComponent* getParent();
    const RectTransformComponent* getParent() const;

private:
    Vector2F anchorPosition = Vector2F::zero();
    AnchorAlignment alignment = AnchorAlignment::LeftTop;
    Vector2F size = Vector2F::zero();
    Vector2F pivot = Vector2F::zero();
    float rotation = 0.0f;
    RectTransformComponent* parent = nullptr;
};
