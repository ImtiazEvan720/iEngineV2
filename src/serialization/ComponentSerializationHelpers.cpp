#include "serialization/ComponentSerializationHelpers.h"

#include "components/Component.h"
#include "misc/TextureAsset.h"
#include "system/AssetManager.h"

#include "tinyxml2.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace ComponentSerializationHelpers {
const char* boolText(bool value) {
    return value ? "true" : "false";
}

bool parseBool(const char* value, bool fallback) {
    if (value == nullptr) {
        return fallback;
    }

    std::string text(value);
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::tolower(character));
                   });

    return text == "true" || text == "1" || text == "yes";
}

tinyxml2::XMLElement* addComponentElement(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const char* type
) {
    tinyxml2::XMLElement* component = document.NewElement("component");
    component->SetAttribute("type", type);
    entityElement.InsertEndChild(component);
    return component;
}

void setComponentEnabledAttribute(tinyxml2::XMLElement& element, const Component& component) {
    element.SetAttribute("enabled", boolText(component.isEnabled()));
}

void applyComponentEnabledAttribute(const tinyxml2::XMLElement& element, Component& component) {
    component.setEnabled(parseBool(element.Attribute("enabled"), true));
}

std::string getTextureAssetName(RenderTextureHandle textureHandle) {
    if (textureHandle == nullptr) {
        return "";
    }

    for (const auto& asset : AssetManager::getInstance().getAssets()) {
        const auto* textureAsset = dynamic_cast<const TextureAsset*>(asset.get());
        if (textureAsset != nullptr && textureAsset->getTextureHandle() == textureHandle) {
            return std::filesystem::path(textureAsset->getPath()).filename().string();
        }
    }

    return "";
}

TextureAsset* getTextureAsset(const tinyxml2::XMLElement& element) {
    const char* textureName = element.Attribute("texture");
    if (textureName == nullptr) {
        return nullptr;
    }

    return AssetManager::getInstance().getTextureAssetByName(textureName);
}

void setSpriteAttributes(tinyxml2::XMLElement& element, const Sprite& sprite) {
    const RenderRect& source = sprite.getSourceRect();
    const Vector2F& size = sprite.getSize();
    const Vector2F& origin = sprite.getOrigin();

    element.SetAttribute("texture", getTextureAssetName(sprite.getTextureHandle()).c_str());
    element.SetAttribute("sourceX", source.x);
    element.SetAttribute("sourceY", source.y);
    element.SetAttribute("sourceWidth", source.width);
    element.SetAttribute("sourceHeight", source.height);
    element.SetAttribute("sizeX", size.x);
    element.SetAttribute("sizeY", size.y);
    element.SetAttribute("originX", origin.x);
    element.SetAttribute("originY", origin.y);
}

Sprite makeSpriteFromAttributes(const tinyxml2::XMLElement& element, TextureAsset& textureAsset) {
    RenderRect source;
    source.x = element.FloatAttribute("sourceX", 0.0f);
    source.y = element.FloatAttribute("sourceY", 0.0f);
    source.width = element.FloatAttribute("sourceWidth", 0.0f);
    source.height = element.FloatAttribute("sourceHeight", 0.0f);

    Sprite sprite(textureAsset.getTextureHandle(), source);
    sprite.setSize(Vector2F(
        element.FloatAttribute("sizeX", source.width),
        element.FloatAttribute("sizeY", source.height)
    ));
    sprite.setOrigin(Vector2F(
        element.FloatAttribute("originX", source.width * 0.5f),
        element.FloatAttribute("originY", source.height * 0.5f)
    ));

    return sprite;
}
}
