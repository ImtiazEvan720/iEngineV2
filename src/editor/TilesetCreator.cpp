#include "editor/TilesetCreator.h"

#include "misc/TextureAsset.h"
#include "system/AssetManager.h"
#include "system/ProjectManager.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "tinyxml2.h"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <system_error>

namespace {
std::filesystem::path findAssetsRoot() {
    for (const auto& asset : AssetManager::getInstance().getAssets()) {
        std::filesystem::path path(asset->getPath());
        std::filesystem::path parent = path.parent_path();

        while (!parent.empty()) {
            if (parent.filename().string() == "Assets") {
                return parent;
            }

            const std::filesystem::path nextParent = parent.parent_path();
            if (nextParent == parent) {
                break;
            }

            parent = nextParent;
        }
    }

    return ProjectManager::getInstance().getAssetsPath();
}

std::vector<TextureAsset*> getLoadedTextureAssets() {
    std::vector<TextureAsset*> textureAssets;

    for (const auto& asset : AssetManager::getInstance().getAssets()) {
        auto* textureAsset = dynamic_cast<TextureAsset*>(asset.get());
        if (textureAsset != nullptr && textureAsset->isLoaded()) {
            textureAssets.push_back(textureAsset);
        }
    }

    return textureAssets;
}

std::string makeDefaultTilesetName(const TextureAsset& textureAsset) {
    std::filesystem::path texturePath(textureAsset.getPath());
    return texturePath.stem().string();
}

std::filesystem::path resolveTilesetOutputPath(const std::string& outputFile) {
    std::filesystem::path outputPath(outputFile);

    if (outputPath.empty()) {
        outputPath = "NewTileset.itile";
    }

    if (!outputPath.is_absolute()) {
        const std::filesystem::path assetsRoot = findAssetsRoot();
        const auto firstPart = outputPath.begin();
        if (firstPart == outputPath.end() || *firstPart != assetsRoot.filename()) {
            outputPath = assetsRoot / outputPath;
        }
    }

    return outputPath.lexically_normal();
}

int colorFloatToByte(float value) {
    return std::clamp(static_cast<int>(std::round(value * 255.0f)), 0, 255);
}

void appendHexByte(std::string& output, int byte) {
    constexpr char HexDigits[] = "0123456789abcdef";
    const int clampedByte = std::clamp(byte, 0, 255);
    output.push_back(HexDigits[(clampedByte >> 4) & 0x0f]);
    output.push_back(HexDigits[clampedByte & 0x0f]);
}

std::string colorBytesToTiledHex(int red, int green, int blue) {
    std::string result;
    result.reserve(6);
    appendHexByte(result, red);
    appendHexByte(result, green);
    appendHexByte(result, blue);
    return result;
}

std::string colorToTiledHex(const float color[3]) {
    return colorBytesToTiledHex(
        colorFloatToByte(color[0]),
        colorFloatToByte(color[1]),
        colorFloatToByte(color[2])
    );
}
}

void TilesetCreator::open() {
    resetForm();
    showPopup = true;
}

bool TilesetCreator::draw(std::string& statusMessage) {
    bool createdTileset = false;

    if (showPopup) {
        ImGui::OpenPopup("Create Tileset");
        showPopup = false;
    }

    if (!ImGui::BeginPopupModal("Create Tileset", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return false;
    }

    std::vector<TextureAsset*> textureAssets = getLoadedTextureAssets();
    if (textureAssets.empty()) {
        ImGui::TextUnformatted("No loaded texture assets found.");
        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
        return false;
    }

    if (selectedTextureIndex < 0
        || selectedTextureIndex >= static_cast<int>(textureAssets.size())) {
        selectedTextureIndex = 0;
    }

    TextureAsset* selectedTexture = textureAssets[static_cast<std::size_t>(selectedTextureIndex)];
    if (selectedTexture != nullptr) {
        const std::string defaultName = makeDefaultTilesetName(*selectedTexture);
        if (tilesetName.empty()) {
            tilesetName = defaultName;
        }

        if (outputFile.empty()) {
            outputFile = defaultName + ".itile";
        }
    }

    const std::string previewText = selectedTexture == nullptr
        ? "None"
        : std::filesystem::path(selectedTexture->getPath()).filename().string();

    if (ImGui::BeginCombo("Texture", previewText.c_str())) {
        for (std::size_t index = 0; index < textureAssets.size(); ++index) {
            TextureAsset* textureAsset = textureAssets[index];
            const std::string label = std::filesystem::path(textureAsset->getPath()).filename().string();
            const bool selected = selectedTextureIndex == static_cast<int>(index);

            if (ImGui::Selectable(label.c_str(), selected)) {
                selectedTextureIndex = static_cast<int>(index);
                resetPreviewImage();

                const std::string defaultName = makeDefaultTilesetName(*textureAsset);
                tilesetName = defaultName;
                outputFile = defaultName + ".itile";
            }

            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::InputText("Tileset Name", &tilesetName);
    ImGui::InputInt("Tile Width", &tileWidth);
    ImGui::InputInt("Tile Height", &tileHeight);
    ImGui::InputText("Output File", &outputFile);
    ImGui::TextWrapped("Save path: %s", resolveTilesetOutputPath(outputFile).string().c_str());

    ImGui::Checkbox("Use Transparency Color", &useTransparencyColor);
    if (!useTransparencyColor) {
        pickingTransparencyColor = false;
    }

    if (!useTransparencyColor) {
        ImGui::BeginDisabled();
    }

    ImGui::ColorEdit3(
        "Transparency Color",
        transparencyColor,
        ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_NoAlpha
    );

    if (ImGui::Button(pickingTransparencyColor ? "Cancel Eyedropper" : "Pick From Image")) {
        pickingTransparencyColor = !pickingTransparencyColor;
    }

    if (pickingTransparencyColor) {
        ImGui::SameLine();
        ImGui::TextUnformatted("Click a pixel in the preview.");
    }

    if (!useTransparencyColor) {
        ImGui::EndDisabled();
    }

    tileWidth = std::max(1, tileWidth);
    tileHeight = std::max(1, tileHeight);

    int columns = 0;
    int rows = 0;
    int tileCount = 0;
    bool validTileGrid = false;

    if (selectedTexture != nullptr) {
        const int imageWidth = selectedTexture->getWidth();
        const int imageHeight = selectedTexture->getHeight();
        validTileGrid =
            imageWidth > 0
            && imageHeight > 0
            && tileWidth > 0
            && tileHeight > 0
            && imageWidth % tileWidth == 0
            && imageHeight % tileHeight == 0;

        if (validTileGrid) {
            columns = imageWidth / tileWidth;
            rows = imageHeight / tileHeight;
            tileCount = columns * rows;
        }

        ImGui::Text("Image size: %dx%d", imageWidth, imageHeight);
        ImGui::Text("Generated grid: %d columns x %d rows, %d tiles", columns, rows, tileCount);
        drawImagePreview(*selectedTexture, statusMessage);
    }

    if (!validTileGrid) {
        ImGui::TextUnformatted("Tile size must divide the texture width and height exactly.");
    }

    const bool canCreate =
        selectedTexture != nullptr
        && !tilesetName.empty()
        && !outputFile.empty()
        && validTileGrid;

    if (!canCreate) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Create and Save")) {
        if (createTilesetFromTexture(
                *selectedTexture,
                outputFile,
                tilesetName,
                tileWidth,
                tileHeight,
                useTransparencyColor,
                transparencyColor,
                statusMessage)) {
            createdTileset = true;
            ImGui::CloseCurrentPopup();
        }
    }

    if (!canCreate) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
    return createdTileset;
}

void TilesetCreator::drawImagePreview(TextureAsset& textureAsset, std::string& statusMessage) {
    const ImTextureID textureId = textureAsset.getImGuiTextureId();
    const int imageWidth = textureAsset.getWidth();
    const int imageHeight = textureAsset.getHeight();

    if (textureId == ImTextureID{} || imageWidth <= 0 || imageHeight <= 0) {
        return;
    }

    const bool imagePixelsLoaded = loadPreviewImage(textureAsset, statusMessage);
    const float availableWidth = std::max(1.0f, ImGui::GetContentRegionAvail().x);
    const float maxPreviewWidth = std::min(availableWidth, 420.0f);
    constexpr float MaxPreviewHeight = 220.0f;
    const float scale = std::clamp(
        std::min(
            maxPreviewWidth / static_cast<float>(imageWidth),
            MaxPreviewHeight / static_cast<float>(imageHeight)
        ),
        0.1f,
        4.0f
    );
    const ImVec2 previewSize(
        static_cast<float>(imageWidth) * scale,
        static_cast<float>(imageHeight) * scale
    );

    ImGui::Separator();
    ImGui::TextUnformatted("Image Preview");
    ImGui::Image(textureId, previewSize);

    const ImVec2 imageMin = ImGui::GetItemRectMin();
    const ImVec2 imageMax = ImGui::GetItemRectMax();

    if (pickingTransparencyColor) {
        ImGui::GetWindowDrawList()->AddRect(
            imageMin,
            imageMax,
            IM_COL32(255, 210, 64, 255),
            0.0f,
            0,
            2.0f
        );
    }

    if (!imagePixelsLoaded) {
        ImGui::TextUnformatted("Could not load image pixels for eyedropper.");
        return;
    }

    if (!ImGui::IsItemHovered()) {
        return;
    }

    const ImVec2 mousePosition = ImGui::GetMousePos();
    const float localX = std::clamp(mousePosition.x - imageMin.x, 0.0f, previewSize.x);
    const float localY = std::clamp(mousePosition.y - imageMin.y, 0.0f, previewSize.y);
    const int pixelX = std::clamp(
        static_cast<int>((localX / previewSize.x) * static_cast<float>(previewImageWidth)),
        0,
        previewImageWidth - 1
    );
    const int pixelY = std::clamp(
        static_cast<int>((localY / previewSize.y) * static_cast<float>(previewImageHeight)),
        0,
        previewImageHeight - 1
    );
    const std::size_t pixelIndex =
        (static_cast<std::size_t>(pixelY) * static_cast<std::size_t>(previewImageWidth)
         + static_cast<std::size_t>(pixelX)) * 4U;

    if (pixelIndex + 2U >= previewPixels.size()) {
        return;
    }

    const int red = previewPixels[pixelIndex];
    const int green = previewPixels[pixelIndex + 1U];
    const int blue = previewPixels[pixelIndex + 2U];
    const std::string hexColor = colorBytesToTiledHex(red, green, blue);

    ImGui::SetTooltip("Pixel: %d, %d\nColor: #%s", pixelX, pixelY, hexColor.c_str());

    if (pickingTransparencyColor && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        transparencyColor[0] = static_cast<float>(red) / 255.0f;
        transparencyColor[1] = static_cast<float>(green) / 255.0f;
        transparencyColor[2] = static_cast<float>(blue) / 255.0f;
        useTransparencyColor = true;
        pickingTransparencyColor = false;
        statusMessage = "Picked transparency color #" + hexColor + ".";
    }
}

bool TilesetCreator::createTilesetFromTexture(
    TextureAsset& textureAsset,
    const std::string& outputFile,
    const std::string& tilesetName,
    int tileWidth,
    int tileHeight,
    bool useTransparencyColor,
    const float transparencyColor[3],
    std::string& statusMessage
) {
    const int imageWidth = textureAsset.getWidth();
    const int imageHeight = textureAsset.getHeight();

    if (tilesetName.empty()
        || tileWidth <= 0
        || tileHeight <= 0
        || imageWidth <= 0
        || imageHeight <= 0
        || imageWidth % tileWidth != 0
        || imageHeight % tileHeight != 0) {
        statusMessage = "Failed to create tileset: invalid tileset settings.";
        return false;
    }

    namespace fs = std::filesystem;

    fs::path outputPath = resolveTilesetOutputPath(outputFile);
    outputPath.replace_extension(".itile");

    std::error_code directoryError;
    fs::create_directories(outputPath.parent_path(), directoryError);
    if (directoryError) {
        statusMessage = "Failed to create tileset directory: " + directoryError.message();
        return false;
    }

    const fs::path texturePath(textureAsset.getPath());
    std::error_code relativeError;
    fs::path imageSource = fs::relative(texturePath, outputPath.parent_path(), relativeError);
    if (relativeError || imageSource.empty()) {
        imageSource = texturePath.filename();
    }

    const int columns = imageWidth / tileWidth;
    const int rows = imageHeight / tileHeight;
    const int tileCount = columns * rows;

    tinyxml2::XMLDocument document;
    document.InsertEndChild(document.NewDeclaration(R"(xml version="1.0" encoding="UTF-8")"));

    tinyxml2::XMLElement* tileset = document.NewElement("itile");
    tileset->SetAttribute("version", "1.0");
    tileset->SetAttribute("name", tilesetName.c_str());
    tileset->SetAttribute("tileWidth", tileWidth);
    tileset->SetAttribute("tileHeight", tileHeight);
    tileset->SetAttribute("tileCount", tileCount);
    tileset->SetAttribute("columns", columns);
    document.InsertEndChild(tileset);

    tinyxml2::XMLElement* image = document.NewElement("image");
    const std::string imageSourceText = imageSource.generic_string();
    image->SetAttribute("source", imageSourceText.c_str());
    if (useTransparencyColor) {
        const std::string transparencyHex = colorToTiledHex(transparencyColor);
        image->SetAttribute("transparency", transparencyHex.c_str());
    }

    image->SetAttribute("width", imageWidth);
    image->SetAttribute("height", imageHeight);
    tileset->InsertEndChild(image);

    const tinyxml2::XMLError result = document.SaveFile(outputPath.string().c_str());
    if (result != tinyxml2::XML_SUCCESS) {
        statusMessage = "Failed to save tileset: " + std::string(document.ErrorStr());
        return false;
    }

    statusMessage = "Created tileset " + outputPath.filename().string() + ".";
    return true;
}

bool TilesetCreator::loadPreviewImage(TextureAsset& textureAsset, std::string& statusMessage) {
    const std::string imagePath = textureAsset.getPath();
    if (previewImagePath == imagePath && !previewPixels.empty()) {
        return true;
    }

    resetPreviewImage();
    previewImagePath = imagePath;

    int width = 0;
    int height = 0;
    int channelCount = 0;
    stbi_uc* pixels = stbi_load(imagePath.c_str(), &width, &height, &channelCount, 4);
    if (pixels == nullptr || width <= 0 || height <= 0) {
        statusMessage = "Failed to load image pixels for eyedropper.";
        if (pixels != nullptr) {
            stbi_image_free(pixels);
        }

        return false;
    }

    const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U;
    previewPixels.assign(pixels, pixels + pixelCount);
    stbi_image_free(pixels);

    previewImageWidth = width;
    previewImageHeight = height;
    return true;
}

void TilesetCreator::resetPreviewImage() {
    previewImagePath.clear();
    previewImageWidth = 0;
    previewImageHeight = 0;
    previewPixels.clear();
}

void TilesetCreator::resetForm() {
    selectedTextureIndex = -1;
    tileWidth = 16;
    tileHeight = 16;
    useTransparencyColor = true;
    pickingTransparencyColor = false;
    transparencyColor[0] = 0.0f;
    transparencyColor[1] = 0.0f;
    transparencyColor[2] = 1.0f / 255.0f;
    tilesetName.clear();
    outputFile.clear();
    resetPreviewImage();
}
