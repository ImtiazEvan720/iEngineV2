#ifndef IENGINEV2_ITEXTURERESOURCE_H
#define IENGINEV2_ITEXTURERESOURCE_H

#include "system/IRenderBackend.h"

#include <string>

class ITextureResource {
public:
    virtual ~ITextureResource() = default;

    virtual bool loadFromFile(const std::string& path) = 0;
    virtual RenderTextureHandle getHandle() const = 0;
};

#endif
