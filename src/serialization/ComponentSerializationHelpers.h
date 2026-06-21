#ifndef IENGINEV2_COMPONENTSERIALIZATIONHELPERS_H
#define IENGINEV2_COMPONENTSERIALIZATIONHELPERS_H

#include "misc/Sprite.h"
#include "system/IRenderBackend.h"

#include <string>

class TextureAsset;

namespace tinyxml2 {
class XMLDocument;
class XMLElement;
}

namespace ComponentSerializationHelpers {
const char* boolText(bool value);
bool parseBool(const char* value, bool fallback = false);

tinyxml2::XMLElement* addComponentElement(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const char* type
);

std::string getTextureAssetName(RenderTextureHandle textureHandle);
TextureAsset* getTextureAsset(const tinyxml2::XMLElement& element);

void setSpriteAttributes(tinyxml2::XMLElement& element, const Sprite& sprite);
Sprite makeSpriteFromAttributes(const tinyxml2::XMLElement& element, TextureAsset& textureAsset);
}

#endif
