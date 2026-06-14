#include "system/sdl/SdlWindowBackend.h"

#include "system/IGuiBackend.h"
#include "system/InputSystem.h"

#include <SDL3/SDL.h>

#include <iostream>

namespace {
InputKey mapKey(SDL_Keycode key) {
    switch (key) {
        case SDLK_W:
            return InputKey::W;
        case SDLK_A:
            return InputKey::A;
        case SDLK_S:
            return InputKey::S;
        case SDLK_D:
            return InputKey::D;
        case SDLK_SPACE:
            return InputKey::Space;
        default:
            return InputKey::Unknown;
    }
}

InputMouseButton mapMouseButton(Uint8 button) {
    switch (button) {
        case SDL_BUTTON_LEFT:
            return InputMouseButton::Left;
        case SDL_BUTTON_RIGHT:
            return InputMouseButton::Right;
        case SDL_BUTTON_MIDDLE:
            return InputMouseButton::Middle;
        default:
            return InputMouseButton::Unknown;
    }
}
}

bool SdlWindowBackend::initialize(
    int width,
    int height,
    const std::string& title,
    int framerateLimit
) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow(
        title.c_str(),
        width,
        height,
        SDL_WINDOW_RESIZABLE
    );

    if (window == nullptr) {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) {
        std::cerr << "Failed to create SDL renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        window = nullptr;
        SDL_Quit();
        return false;
    }

    frameDelayMilliseconds = framerateLimit > 0 ? 1000 / framerateLimit : 0;
    open = true;
    return true;
}

void SdlWindowBackend::shutdown() {
    if (renderer != nullptr) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    if (window != nullptr) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    open = false;
    SDL_Quit();
}

bool SdlWindowBackend::isOpen() const {
    return open;
}

void SdlWindowBackend::close() {
    open = false;
}

void SdlWindowBackend::pollEvents(InputSystem& inputSystem, IGuiBackend* guiBackend) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (guiBackend != nullptr) {
            guiBackend->processNativeEvent(&event);
        }

        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            open = false;
        } else if (event.type == SDL_EVENT_KEY_DOWN) {
            inputSystem.processKeyPressed(mapKey(event.key.key));
        } else if (event.type == SDL_EVENT_KEY_UP) {
            inputSystem.processKeyReleased(mapKey(event.key.key));
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            inputSystem.processMousePressed(
                mapMouseButton(event.button.button),
                static_cast<int>(event.button.x),
                static_cast<int>(event.button.y)
            );
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            inputSystem.processMouseReleased(
                mapMouseButton(event.button.button),
                static_cast<int>(event.button.x),
                static_cast<int>(event.button.y)
            );
        }
    }
}

void SdlWindowBackend::beginFrame(const RenderColor& clearColor) {
    if (renderer == nullptr) {
        return;
    }

    SDL_SetRenderDrawColor(renderer, clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    SDL_RenderClear(renderer);
}

void SdlWindowBackend::endFrame() {
    if (renderer == nullptr) {
        return;
    }

    SDL_RenderPresent(renderer);

#ifndef __EMSCRIPTEN__
    if (frameDelayMilliseconds > 0) {
        SDL_Delay(static_cast<Uint32>(frameDelayMilliseconds));
    }
#endif
}

RenderRect SdlWindowBackend::getViewport() const {
    if (renderer == nullptr) {
        return RenderRect{};
    }

    int width = 0;
    int height = 0;
    SDL_GetCurrentRenderOutputSize(renderer, &width, &height);

    return RenderRect{
        0.0f,
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height)
    };
}

SDL_Window* SdlWindowBackend::getWindow() {
    return window;
}

SDL_Renderer* SdlWindowBackend::getRenderer() {
    return renderer;
}
