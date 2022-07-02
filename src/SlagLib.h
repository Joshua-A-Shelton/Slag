#ifndef SLAGLIB_SLAGLIB_H
#define SLAGLIB_SLAGLIB_H

#include <cstddef>
#include "GraphicsCard.h"

namespace slag
{
    void initialize(bool includeValidationLayers);
    void destroy();
    size_t graphicsCardCount();
    GraphicsCard* getGraphicsCard(size_t index);
}

#endif //SLAGLIB_SLAGLIB_H