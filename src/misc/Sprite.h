#ifndef IENGINEV2_SPRITE_H
#define IENGINEV2_SPRITE_H

#include "math/Vector2F.h"
#include "system/IRenderBackend.h"

class Sprite {
public:
    Sprite(RenderTextureHandle texture, const RenderRect& sourceRect);

    RenderTextureHandle getTextureHandle() const;
    const RenderRect& getSourceRect() const;
    const Vector2F& getOrigin() const;
    const Vector2F& getSize() const;

    void setTextureHandle(RenderTextureHandle texture);
    void setSourceRect(const RenderRect& sourceRect);
    void setOrigin(const Vector2F& origin);
    void setSize(const Vector2F& size);

private:
    RenderTextureHandle texture;
    RenderRect sourceRect;
    Vector2F origin;
    Vector2F size;
};

#endif
