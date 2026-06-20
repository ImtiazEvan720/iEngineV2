#ifndef IENGINEV2_EDITORCAMERA_H
#define IENGINEV2_EDITORCAMERA_H

#include "system/IRenderBackend.h"

#include <string>

class EditorCamera {
public:
    void beginFrame();
    bool handleZoom(bool enabled, std::string& statusMessage);
    bool handlePan(bool enabled);

    bool isPanActive() const;
    RenderRect getViewport() const;

private:
    bool consumedPinchZoomThisFrame = false;
};

#endif
