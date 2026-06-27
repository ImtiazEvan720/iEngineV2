#pragma once

#include "misc/Sprite.h"
#include "system/IRenderBackend.h"

#include <string>

class TextureAsset;
class Component;

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

void setComponentEnabledAttribute(tinyxml2::XMLElement& element, const Component& component);
void applyComponentEnabledAttribute(const tinyxml2::XMLElement& element, Component& component);

std::string getTextureAssetName(RenderTextureHandle textureHandle);
TextureAsset* getTextureAsset(const tinyxml2::XMLElement& element);

void setSpriteAttributes(tinyxml2::XMLElement& element, const Sprite& sprite);
Sprite makeSpriteFromAttributes(const tinyxml2::XMLElement& element, TextureAsset& textureAsset);
}
