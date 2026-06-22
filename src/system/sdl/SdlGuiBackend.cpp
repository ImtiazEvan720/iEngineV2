#include "system/sdl/SdlGuiBackend.h"

#include "system/InputSystem.h"
#include "system/sdl/SdlWindowBackend.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#include <SDL3/SDL.h>

bool SdlGuiBackend::initialize(IWindowBackend& backend) {
#ifndef IENGINE_WITH_EDITOR
    (void)backend;
    initialized = true;
    return true;
#else
    windowBackend = dynamic_cast<SdlWindowBackend*>(&backend);
    if (windowBackend == nullptr
        || windowBackend->getWindow() == nullptr
        || windowBackend->getRenderer() == nullptr) {
        initialized = false;
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    initialized = ImGui_ImplSDL3_InitForSDLRenderer(
        windowBackend->getWindow(),
        windowBackend->getRenderer()
    ) && ImGui_ImplSDLRenderer3_Init(windowBackend->getRenderer());

    return initialized;
#endif
}

void SdlGuiBackend::processNativeEvent(const void* event) {
#ifndef IENGINE_WITH_EDITOR
    (void)event;
#else
    if (!initialized || event == nullptr) {
        return;
    }

    ImGui_ImplSDL3_ProcessEvent(static_cast<const SDL_Event*>(event));
#endif
}

void SdlGuiBackend::update(float deltaTime) {
#ifndef IENGINE_WITH_EDITOR
    (void)deltaTime;
#else

    if (!initialized) {
        return;
    }

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    levelEditor.updateEditorOnly(deltaTime);
#endif
}

void SdlGuiBackend::render(const InputSystem& inputSystem) {
#ifndef IENGINE_WITH_EDITOR
    (void)inputSystem;
#else
    if (!initialized || windowBackend == nullptr) {
        return;
    }

    levelEditor.draw(inputSystem, windowBackend->getViewport().width);

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(
        ImGui::GetDrawData(),
        windowBackend->getRenderer()
    );
#endif
}

void SdlGuiBackend::shutdown() {
#ifdef IENGINE_WITH_EDITOR
    if (initialized) {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
#endif

    initialized = false;
    windowBackend = nullptr;
}
