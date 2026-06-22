#pragma once

#include "system/IRenderBackend.h"

#include "imgui.h"

#include <string>

class ITextureResource {
public:
    virtual ~ITextureResource() = default;

    virtual bool loadFromFile(const std::string& path) = 0;
    virtual RenderTextureHandle getHandle() const = 0;
    virtual ImTextureID getImGuiTextureId() const = 0;
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
};
