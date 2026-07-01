#pragma once

#include <string>

class Entity;
class TransformComponent;
class PlayerCameraComponent;

class EditorPlayCameraPreviewPanel
{
public:
    EditorPlayCameraPreviewPanel() = default;
    ~EditorPlayCameraPreviewPanel() = default;

    void draw(std::string& statusMessage);

private:
    int previewCameraWidth = 640;
    int previewCameraHeight = 480;

    const Entity* findMainCameraEntity() const;
};
