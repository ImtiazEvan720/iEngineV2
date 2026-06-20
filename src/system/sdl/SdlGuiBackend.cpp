#include "system/sdl/SdlGuiBackend.h"

#include "system/InputSystem.h"
#include "system/sdl/SdlWindowBackend.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#include <SDL3/SDL.h>

bool SdlGuiBackend::initialize(IWindowBackend& backend) {
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
}

void SdlGuiBackend::processNativeEvent(const void* event) {
    if (!initialized || event == nullptr) {
        return;
    }

    ImGui_ImplSDL3_ProcessEvent(static_cast<const SDL_Event*>(event));
}

void SdlGuiBackend::update(float deltaTime) {
    (void)deltaTime;

    if (!initialized) {
        return;
    }

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

#ifdef IENGINE_WITH_EDITOR
    levelEditor.updateEditorOnly(deltaTime);
#endif
}

void SdlGuiBackend::render(const InputSystem& inputSystem) {
    if (!initialized || windowBackend == nullptr) {
        return;
    }

#ifdef IENGINE_WITH_EDITOR
    levelEditor.draw(inputSystem, windowBackend->getViewport().width);
#else
    (void)inputSystem;
#endif

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(
        ImGui::GetDrawData(),
        windowBackend->getRenderer()
    );
}

void SdlGuiBackend::shutdown() {
    if (initialized) {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }

    initialized = false;
    windowBackend = nullptr;
}
