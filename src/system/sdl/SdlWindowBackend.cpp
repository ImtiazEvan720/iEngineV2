#include "system/sdl/SdlWindowBackend.h"

#include "system/IGuiBackend.h"
#include "system/InputSystem.h"
#include "system/RawInputSystem.h"

#include <SDL3/SDL.h>

#include <cmath>
#include <iostream>
#include <limits>

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

RawKey mapRawKey(SDL_Keycode key) {
    switch (key) {
        case SDLK_W:
            return RawKey::W;
        case SDLK_A:
            return RawKey::A;
        case SDLK_S:
            return RawKey::S;
        case SDLK_D:
            return RawKey::D;
        case SDLK_SPACE:
            return RawKey::Space;
        case SDLK_ESCAPE:
            return RawKey::Escape;
        default:
            return RawKey::Unknown;
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

RawMouseButton mapRawMouseButton(Uint8 button) {
    switch (button) {
        case SDL_BUTTON_LEFT:
            return RawMouseButton::Left;
        case SDL_BUTTON_RIGHT:
            return RawMouseButton::Right;
        case SDL_BUTTON_MIDDLE:
            return RawMouseButton::Middle;
        default:
            return RawMouseButton::Unknown;
    }
}

int toRawTouchId(SDL_FingerID fingerId) {
    constexpr SDL_FingerID maxTouchId = static_cast<SDL_FingerID>(std::numeric_limits<int>::max());
    return static_cast<int>(fingerId % maxTouchId);
}

void getTouchPosition(SDL_Window* window, const SDL_TouchFingerEvent& touchEvent, float& x, float& y) {
    int width = 0;
    int height = 0;
    if (window != nullptr) {
        SDL_GetWindowSize(window, &width, &height);
    }

    x = touchEvent.x * static_cast<float>(width);
    y = touchEvent.y * static_cast<float>(height);
}
}

bool SdlWindowBackend::initialize(
    int width,
    int height,
    const std::string& title,
    int framerateLimit
) {
#ifdef IENGINE_ANDROID
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
#endif

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
    RawInputSystem& rawInputSystem = RawInputSystem::getInstance();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (guiBackend != nullptr) {
            guiBackend->processNativeEvent(&event);
        }

        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            open = false;
        } else if (event.type == SDL_EVENT_PINCH_UPDATE) {
            if (std::isfinite(event.pinch.scale) && event.pinch.scale > 0.0f) {
                pendingPinchZoomFactor *= event.pinch.scale;
            }
        } else if (event.type == SDL_EVENT_KEY_DOWN) {
            inputSystem.processKeyPressed(mapKey(event.key.key));
            rawInputSystem.setKeyDown(mapRawKey(event.key.key));
        } else if (event.type == SDL_EVENT_KEY_UP) {
            inputSystem.processKeyReleased(mapKey(event.key.key));
            rawInputSystem.setKeyUp(mapRawKey(event.key.key));
        } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
            rawInputSystem.setMousePosition(
                static_cast<int>(event.motion.x),
                static_cast<int>(event.motion.y)
            );
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            inputSystem.processMousePressed(
                mapMouseButton(event.button.button),
                static_cast<int>(event.button.x),
                static_cast<int>(event.button.y)
            );
            rawInputSystem.setMouseButtonDown(
                mapRawMouseButton(event.button.button),
                static_cast<int>(event.button.x),
                static_cast<int>(event.button.y)
            );
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            inputSystem.processMouseReleased(
                mapMouseButton(event.button.button),
                static_cast<int>(event.button.x),
                static_cast<int>(event.button.y)
            );
            rawInputSystem.setMouseButtonUp(
                mapRawMouseButton(event.button.button),
                static_cast<int>(event.button.x),
                static_cast<int>(event.button.y)
            );
        } else if (event.type == SDL_EVENT_FINGER_DOWN) {
            float x = 0.0f;
            float y = 0.0f;
            getTouchPosition(window, event.tfinger, x, y);
            rawInputSystem.setTouchDown(toRawTouchId(event.tfinger.fingerID), x, y);
        } else if (event.type == SDL_EVENT_FINGER_MOTION) {
            float x = 0.0f;
            float y = 0.0f;
            getTouchPosition(window, event.tfinger, x, y);
            rawInputSystem.setTouchMove(toRawTouchId(event.tfinger.fingerID), x, y);
        } else if (event.type == SDL_EVENT_FINGER_UP || event.type == SDL_EVENT_FINGER_CANCELED) {
            float x = 0.0f;
            float y = 0.0f;
            getTouchPosition(window, event.tfinger, x, y);
            rawInputSystem.setTouchUp(toRawTouchId(event.tfinger.fingerID), x, y);
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

#if !defined(__EMSCRIPTEN__) && !defined(IENGINE_IOS)
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

float SdlWindowBackend::consumePendingPinchZoomFactor() {
    const float zoomFactor = pendingPinchZoomFactor;
    pendingPinchZoomFactor = 1.0f;

    if (!std::isfinite(zoomFactor) || zoomFactor <= 0.0f) {
        return 1.0f;
    }

    return zoomFactor;
}

SDL_Window* SdlWindowBackend::getWindow() {
    return window;
}

SDL_Renderer* SdlWindowBackend::getRenderer() {
    return renderer;
}
