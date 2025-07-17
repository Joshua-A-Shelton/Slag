#include "GraphicsAPIEnvironment.h"

namespace slag
{
    GraphicsAPIEnvironment* GraphicsAPIEnvironment::graphicsAPIEnvironment()
    {
        return _currentEnv;
    }

    void GraphicsAPIEnvironment::SetAsCurrentEnv()
    {
        _currentEnv = this;
    }
}
