#include "editor/EditorPlayCameraPreviewPanel.h"

#include "Entity.h"
#include "components/PlayerCameraComponent.h"
#include "components/TransformComponent.h"
#include "misc/Level.h"
#include "system/Renderer.h"

#include "imgui.h"

void EditorPlayCameraPreviewPanel::draw(std::string& statusMessage)
{
    if (!ImGui::Begin("Camera Preview")) {
        ImGui::End();
        return;
    }

    const Entity* cameraEntity = findMainCameraEntity();

    if (cameraEntity == nullptr)
    {
        statusMessage = "No MainCamera entity. Cannot draw camera preview panel.";
        ImGui::TextUnformatted(statusMessage.c_str());
        ImGui::End();
        return;
    }

    const TransformComponent* transform = cameraEntity->getComponent<TransformComponent>();
    const PlayerCameraComponent* camera = cameraEntity->getComponent<PlayerCameraComponent>();
    if (transform == nullptr || camera == nullptr)
    {
        statusMessage = "MainCamera missing TransformComponent or PlayerCameraComponent.";
        ImGui::TextUnformatted(statusMessage.c_str());
        ImGui::End();
        return;
    }
    
    if (!camera->isEnabled())
    {
        statusMessage = "MainCamera entity is disabled. Cannot draw camera preview panel.";
        ImGui::TextUnformatted(statusMessage.c_str());
        ImGui::End();
        return;
    }

    ImTextureID previewTexture = Renderer::getInstance().renderCameraPreview(
        *transform,
        *camera,
        previewCameraWidth,
        previewCameraHeight);

    if (previewTexture == ImTextureID{})
    {
        ImGui::TextUnformatted("Preview unavailable.");
    }
    else
    {
        ImGui::Image(previewTexture, ImVec2(previewCameraWidth, previewCameraHeight));
    }
    ImGui::End();
}

const Entity *EditorPlayCameraPreviewPanel::findMainCameraEntity() const
{
    Level &level = Level::getCurrentLevel();

    for (const Entity &entity : level.getEntities())
    {
        if (entity.isDestroyed() || !entity.isEnabled())
        {
            continue;
        }

        if (entity.getTag() == "MainCamera")
        {
            return &entity;
        }
    }

    return nullptr;
}
