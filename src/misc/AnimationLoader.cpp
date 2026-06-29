#include "misc/AnimationLoader.h"

#include "misc/Sprite.h"
#include "misc/TextureAsset.h"
#include "system/AssetManager.h"
#include "system/ProjectManager.h"
#include "system/Renderer.h"

#include "tinyxml2.h"

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {
struct LoaderTileset {
    int tileWidth = 0;
    int tileHeight = 0;
    int columns = 0;
    int tileCount = 0;
    TextureAsset* textureAsset = nullptr;
};

std::string getAttribute(
    const tinyxml2::XMLElement* element,
    const char* name,
    const std::string& fallback = ""
) {
    if (element == nullptr) {
        return fallback;
    }

    const char* value = element->Attribute(name);
    return value == nullptr ? fallback : value;
}

int getIntAttributeWithFallback(
    const tinyxml2::XMLElement* element,
    const char* primaryName,
    const char* fallbackName,
    int fallback = 0
) {
    if (element == nullptr) {
        return fallback;
    }

    if (element->FindAttribute(primaryName) != nullptr) {
        return element->IntAttribute(primaryName, fallback);
    }

    return element->IntAttribute(fallbackName, fallback);
}

std::filesystem::path resolveRelativePath(
    const std::filesystem::path& baseFile,
    const std::string& relativePath
) {
    std::filesystem::path path(relativePath);
    if (path.is_absolute()) {
        return path.lexically_normal();
    }

    return (baseFile.parent_path() / path).lexically_normal();
}

std::filesystem::path findExistingAnimationPath(const std::string& sourcePath) {
    namespace fs = std::filesystem;

    const fs::path path(sourcePath);
    std::error_code error;
    if (fs::exists(path, error)) {
        return path.lexically_normal();
    }

    const fs::path assetsPath = ProjectManager::getInstance().getAssetsPath();
    if (!assetsPath.empty()) {
        if (!path.empty() && *path.begin() == "Assets") {
            fs::path relativeToAssets;
            for (auto iterator = std::next(path.begin()); iterator != path.end(); ++iterator) {
                relativeToAssets /= *iterator;
            }

            const fs::path resolved = (assetsPath / relativeToAssets).lexically_normal();
            if (fs::exists(resolved, error)) {
                return resolved;
            }
        }

        const fs::path resolved = (assetsPath / path).lexically_normal();
        if (fs::exists(resolved, error)) {
            return resolved;
        }
    }

    return path.lexically_normal();
}

std::filesystem::path findFileInAssetsByName(const std::string& filename) {
    namespace fs = std::filesystem;

    const fs::path assetsPath = ProjectManager::getInstance().getAssetsPath();
    if (assetsPath.empty()) {
        return {};
    }

    std::error_code error;
    if (!fs::exists(assetsPath, error) || !fs::is_directory(assetsPath, error)) {
        return {};
    }

    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(assetsPath, error)) {
        if (error) {
            break;
        }

        if (entry.is_regular_file(error) && entry.path().filename().string() == filename) {
            return entry.path().lexically_normal();
        }
    }

    return {};
}

std::filesystem::path resolveTilesetPath(
    const std::filesystem::path& animationPath,
    const std::string& tilesetReference
) {
    namespace fs = std::filesystem;

    fs::path resolved = resolveRelativePath(animationPath, tilesetReference);
    std::error_code error;
    if (fs::exists(resolved, error)) {
        return resolved;
    }

    const fs::path foundInAssets = findFileInAssetsByName(fs::path(tilesetReference).filename().string());
    if (!foundInAssets.empty()) {
        return foundInAssets;
    }

    return resolved;
}

bool loadTileset(
    const std::filesystem::path& tilesetPath,
    LoaderTileset& tileset,
    std::string& errorMessage
) {
    tinyxml2::XMLDocument document;
    if (document.LoadFile(tilesetPath.string().c_str()) != tinyxml2::XML_SUCCESS) {
        errorMessage = "Failed to load animation tileset: " + std::string(document.ErrorStr());
        return false;
    }

    const tinyxml2::XMLElement* tilesetRoot = document.FirstChildElement("itile");
    if (tilesetRoot == nullptr) {
        errorMessage = "Animation tileset is missing itile root: " + tilesetPath.filename().string();
        return false;
    }

    tileset.tileWidth = getIntAttributeWithFallback(tilesetRoot, "tileWidth", "tilewidth");
    tileset.tileHeight = getIntAttributeWithFallback(tilesetRoot, "tileHeight", "tileheight");
    tileset.columns = tilesetRoot->IntAttribute("columns");
    tileset.tileCount = getIntAttributeWithFallback(tilesetRoot, "tileCount", "tilecount");

    const tinyxml2::XMLElement* image = tilesetRoot->FirstChildElement("image");
    const std::string imageSource = getAttribute(image, "source");
    const std::string imageFilename = std::filesystem::path(imageSource).filename().string();
    tileset.textureAsset = AssetManager::getInstance().getTextureAssetByName(imageFilename);

    if (tileset.tileWidth <= 0 || tileset.tileHeight <= 0 || tileset.columns <= 0 || tileset.tileCount <= 0) {
        errorMessage = "Animation tileset metadata is incomplete: " + tilesetPath.filename().string();
        return false;
    }

    if (tileset.textureAsset == nullptr || tileset.textureAsset->getTextureHandle() == nullptr) {
        errorMessage = "Animation tileset texture is not loaded: " + imageFilename;
        return false;
    }

    return true;
}

std::optional<Sprite> createSpriteFromTile(
    const LoaderTileset& tileset,
    int tileId,
    float renderScale,
    std::string& errorMessage
) {
    if (tileId < 0 || tileId >= tileset.tileCount) {
        errorMessage = "Animation frame has invalid tile id: " + std::to_string(tileId);
        return std::nullopt;
    }

    const int column = tileId % tileset.columns;
    const int row = tileId / tileset.columns;
    const float sourceX = static_cast<float>(column * tileset.tileWidth);
    const float sourceY = static_cast<float>(row * tileset.tileHeight);

    Sprite sprite(
        tileset.textureAsset->getTextureHandle(),
        RenderRect{
            sourceX,
            sourceY,
            static_cast<float>(tileset.tileWidth),
            static_cast<float>(tileset.tileHeight)
        }
    );
    sprite.setSize(Vector2F(
        static_cast<float>(tileset.tileWidth) * renderScale,
        static_cast<float>(tileset.tileHeight) * renderScale
    ));
    sprite.setOrigin(Vector2F::zero());

    return sprite;
}
}

bool AnimationLoader::loadFromFile(
    const std::string& path,
    Animation& animation,
    std::string& errorMessage
) {
    const std::filesystem::path animationPath = findExistingAnimationPath(path);

    tinyxml2::XMLDocument document;
    if (document.LoadFile(animationPath.string().c_str()) != tinyxml2::XML_SUCCESS) {
        errorMessage = "Failed to load animation file: " + std::string(document.ErrorStr());
        return false;
    }

    const tinyxml2::XMLElement* root = document.FirstChildElement("ianim");
    if (root == nullptr) {
        errorMessage = "Animation file is missing ianim root: " + animationPath.filename().string();
        return false;
    }

    const char* tilesetReference = root->Attribute("tileset");
    if (tilesetReference == nullptr || tilesetReference[0] == '\0') {
        errorMessage = "Animation file is missing tileset reference: " + animationPath.filename().string();
        return false;
    }

    LoaderTileset tileset;
    const std::filesystem::path tilesetPath = resolveTilesetPath(animationPath, tilesetReference);
    if (!loadTileset(tilesetPath, tileset, errorMessage)) {
        return false;
    }

    const float renderScale = Renderer::getInstance().getRenderScale();
    const float tileWidth = static_cast<float>(tileset.tileWidth) * renderScale;
    const float tileHeight = static_cast<float>(tileset.tileHeight) * renderScale;

    Animation loadedAnimation;
    for (const tinyxml2::XMLElement* frameElement = root->FirstChildElement("frame");
         frameElement != nullptr;
         frameElement = frameElement->NextSiblingElement("frame")) {
        const int durationMs = std::max(1, frameElement->IntAttribute("duration", 100));
        const int widthInTiles = std::max(1, frameElement->IntAttribute("width", 1));
        const int heightInTiles = std::max(1, frameElement->IntAttribute("height", 1));
        const Vector2F frameCenter(
            static_cast<float>(widthInTiles) * tileWidth * 0.5f,
            static_cast<float>(heightInTiles) * tileHeight * 0.5f
        );
        std::vector<AnimationFrameSprite> frameSprites;

        for (const tinyxml2::XMLElement* tileElement = frameElement->FirstChildElement("tile");
             tileElement != nullptr;
             tileElement = tileElement->NextSiblingElement("tile")) {
            const int tileId = tileElement->IntAttribute("id", -1);
            const int tileX = tileElement->IntAttribute("x", 0);
            const int tileY = tileElement->IntAttribute("y", 0);

            std::optional<Sprite> sprite = createSpriteFromTile(tileset, tileId, renderScale, errorMessage);
            if (!sprite.has_value()) {
                return false;
            }

            frameSprites.emplace_back(
                *sprite,
                Vector2F(
                    static_cast<float>(tileX) * tileWidth - frameCenter.x,
                    static_cast<float>(tileY) * tileHeight - frameCenter.y
                )
            );
        }

        if (frameSprites.empty()) {
            continue;
        }

        loadedAnimation.addFrame(
            std::move(frameSprites),
            Vector2F(
                static_cast<float>(widthInTiles) * tileWidth,
                static_cast<float>(heightInTiles) * tileHeight
            ),
            static_cast<float>(durationMs) / 1000.0f
        );
    }

    if (!loadedAnimation.hasFrames()) {
        errorMessage = "Animation file has no valid frames: " + animationPath.filename().string();
        return false;
    }

    animation = loadedAnimation;
    errorMessage.clear();
    return true;
}
