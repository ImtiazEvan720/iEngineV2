#include "editor/components/CanvasComponentDrawer.h"

#include "Entity.h"
#include "components/CanvasComponent.h"
#include "math/Vector2F.h"
#include "system/Renderer.h"

#include "imgui.h"

#include <algorithm>
#include <cstdint>

namespace {
std::uint8_t toColorChannel(float value)
{
    const float clampedValue = std::clamp(value, 0.0f, 1.0f);
    return static_cast<std::uint8_t>((clampedValue * 255.0f) + 0.5f);
}

RenderColor toRenderColor(const float color[4])
{
    return RenderColor{
        toColorChannel(color[0]),
        toColorChannel(color[1]),
        toColorChannel(color[2]),
        toColorChannel(color[3])};
}

void copyRenderColorToFloats(const RenderColor &color, float outColor[4])
{
    outColor[0] = color.r / 255.0f;
    outColor[1] = color.g / 255.0f;
    outColor[2] = color.b / 255.0f;
    outColor[3] = color.a / 255.0f;
}
}

void CanvasComponentDrawer::draw(
    Entity &entity,
    Component &component,
    std::string &statusMessage)
{
    auto *canvasComponent = dynamic_cast<CanvasComponent *>(&component);
    if (canvasComponent == nullptr)
    {
        ImGui::TextDisabled("Invalid CanvasComponent.");
        return;
    }

    if (editState.entityId != entity.getId())
    {
        screenEyedropActive = false;
        screenEyedropTargetEntityId = -1;
        screenEyedropArmedFrame = -1;

        editState.entityId = entity.getId();
        const Vector2F &referenceResolution = canvasComponent->getReferenceResolution();
        editState.referenceResolution[0] = referenceResolution.x;
        editState.referenceResolution[1] = referenceResolution.y;
        editState.sortingOrder = canvasComponent->getSortingOrder();
        editState.opacity = canvasComponent->getOpacity();
        editState.scale = canvasComponent->getScale();
        editState.canvasColor = canvasComponent->getCanvasColor();
    }

    ImGui::InputFloat2("Reference Resolution", editState.referenceResolution, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        editState.referenceResolution[0] = std::max(1.0f, editState.referenceResolution[0]);
        editState.referenceResolution[1] = std::max(1.0f, editState.referenceResolution[1]);
        canvasComponent->setReferenceResolution(Vector2F(
            editState.referenceResolution[0],
            editState.referenceResolution[1]));
        statusMessage = "Updated CanvasComponent reference resolution.";
    }

    ImGui::InputInt("Sorting Order", &editState.sortingOrder);
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        canvasComponent->setSortingOrder(editState.sortingOrder);
        statusMessage = "Updated CanvasComponent sorting order.";
    }

    ImGui::InputFloat("Scale", &editState.scale, 0.0f, 0.0f, "%.3f");
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        editState.scale = std::max(0.001f, editState.scale);
        canvasComponent->setScale(editState.scale);
        statusMessage = "Updated CanvasComponent scale.";
    }

    ImGui::SliderFloat("Opacity", &editState.opacity, 0.0f, 1.0f, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        canvasComponent->setOpacity(editState.opacity);
        statusMessage = "Updated CanvasComponent opacity.";
    }

    float canvasColor[4];
    copyRenderColorToFloats(editState.canvasColor, canvasColor);

    if (ImGui::ColorEdit4(
            "Canvas Color",
            canvasColor,
            ImGuiColorEditFlags_AlphaBar |
                ImGuiColorEditFlags_AlphaPreviewHalf |
                ImGuiColorEditFlags_DisplayRGB))
    {
        editState.canvasColor = toRenderColor(canvasColor);
        canvasComponent->setCanvasColor(editState.canvasColor);
        statusMessage = "Updated CanvasComponent canvas color.";
    }


    ImGui::SameLine();
    if (ImGui::Button(screenEyedropActive ? "Cancel Eyedrop" : "Eyedrop Screen"))
    {
        if (screenEyedropActive)
        {
            screenEyedropActive = false;
            screenEyedropTargetEntityId = -1;
            screenEyedropArmedFrame = -1;
            statusMessage = "Canceled canvas color eyedropper.";
        }
        else
        {
            screenEyedropActive = true;
            screenEyedropTargetEntityId = entity.getId();
            screenEyedropArmedFrame = ImGui::GetFrameCount();
            statusMessage = "Click inside the app window to pick a canvas color.";
        }
    }

    if (!screenEyedropActive)
    {
        return;
    }

    ImGui::TextDisabled("Eyedropper active. Click in the app window, or press Esc to cancel.");

    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        screenEyedropActive = false;
        screenEyedropTargetEntityId = -1;
        screenEyedropArmedFrame = -1;
        statusMessage = "Canceled canvas color eyedropper.";
        return;
    }

    if (screenEyedropTargetEntityId != entity.getId())
    {
        screenEyedropActive = false;
        screenEyedropTargetEntityId = -1;
        screenEyedropArmedFrame = -1;
        statusMessage = "Canceled canvas color eyedropper because selection changed.";
        return;
    }

    if (ImGui::GetFrameCount() <= screenEyedropArmedFrame ||
        !ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        return;
    }

    const ImVec2 mousePosition = ImGui::GetMousePos();
    RenderColor pickedColor;
    if (!Renderer::getInstance().pickScreenColor(
            static_cast<int>(mousePosition.x),
            static_cast<int>(mousePosition.y),
            pickedColor))
    {
        statusMessage = "Failed to pick canvas color from screen.";
        screenEyedropActive = false;
        screenEyedropTargetEntityId = -1;
        screenEyedropArmedFrame = -1;
        return;
    }

    editState.canvasColor = pickedColor;
    canvasComponent->setCanvasColor(editState.canvasColor);
    screenEyedropActive = false;
    screenEyedropTargetEntityId = -1;
    screenEyedropArmedFrame = -1;
    statusMessage = "Picked CanvasComponent canvas color from screen.";
}
