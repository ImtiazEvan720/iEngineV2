#include "Application.h"

#if defined(IENGINE_IOS) || defined(IENGINE_ANDROID)
#include <SDL3/SDL_main.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#ifdef __EMSCRIPTEN__
namespace {
void runApplicationFrame(void* userData) {
    Application* application = static_cast<Application*>(userData);
    application->tick();

    if (!application->isRunning()) {
        application->shutdown();
        emscripten_cancel_main_loop();
    }
}
}
#endif

int main(int argc, char* argv[]) {
#ifdef __EMSCRIPTEN__
    static Application application;
    if (!application.initialize(argc, argv)) {
        return 1;
    }

    emscripten_set_main_loop_arg(runApplicationFrame, &application, 0, true);
    return 0;
#else
    Application application;
    if (!application.initialize(argc, argv)) {
        return 1;
    }

    while (application.isRunning()) {
        application.tick();
    }

    application.shutdown();
    return 0;
#endif
}
