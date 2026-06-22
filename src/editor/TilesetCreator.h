#pragma once

#include <string>
#include <vector>

class TextureAsset;

class TilesetCreator {
public:
    void open();
    bool draw(std::string& statusMessage);

private:
    void drawImagePreview(TextureAsset& textureAsset, std::string& statusMessage);
    bool createTilesetFromTexture(
        TextureAsset& textureAsset,
        const std::string& outputFile,
        const std::string& tilesetName,
        int tileWidth,
        int tileHeight,
        bool useTransparencyColor,
        const float transparencyColor[3],
        std::string& statusMessage
    );
    bool loadPreviewImage(TextureAsset& textureAsset, std::string& statusMessage);
    void resetPreviewImage();
    void resetForm();

    bool showPopup = false;
    bool useTransparencyColor = true;
    bool pickingTransparencyColor = false;
    float transparencyColor[3] = {0.0f, 0.0f, 1.0f / 255.0f};
    int selectedTextureIndex = -1;
    int tileWidth = 16;
    int tileHeight = 16;
    std::string tilesetName;
    std::string outputFile;
    std::string previewImagePath;
    int previewImageWidth = 0;
    int previewImageHeight = 0;
    std::vector<unsigned char> previewPixels;
};
