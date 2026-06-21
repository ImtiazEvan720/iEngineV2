#include "serialization/ComponentSerializer.h"

bool IComponentSerializer::loadBeforeOtherComponents() const {
    return false;
}

bool IComponentSerializer::requiresTransformBeforeLoad() const {
    return false;
}
