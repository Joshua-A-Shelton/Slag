#include "FramebufferDescription.h"

namespace slag
{
    FramebufferDescription &FramebufferDescription::addColorTarget(Pixels::PixelFormat format)
    {
        _colorFormats.push_back(format);
        return *this;
    }

    FramebufferDescription &FramebufferDescription::setDepthTarget(Pixels::PixelFormat format)
    {
        _depthFormat = format;
        return *this;
    }

    size_t FramebufferDescription::colorTargetCount()
    {
        return _colorFormats.size();
    }

    Pixels::PixelFormat FramebufferDescription::colorFormat(size_t index)
    {
        return _colorFormats[index];
    }

    Pixels::PixelFormat FramebufferDescription::depthFormat()
    {
        return _depthFormat;
    }
} // slag