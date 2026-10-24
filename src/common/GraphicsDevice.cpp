#include "common/Objects.h"

#include <slag/GraphicsDevice.h>

namespace slag {

const GraphicsDeviceInfo& GraphicsDevice::info() const
{
    return static_cast<const GraphicsDeviceCommon*>(this)->info;
}

uint64_t GraphicsDevice::userData() const
{
    return static_cast<const GraphicsDeviceCommon*>(this)->userData;
}

void GraphicsDevice::setUserData(uint64_t value)
{
    static_cast<GraphicsDeviceCommon*>(this)->userData = value;
}

} // namespace slag
