#include "FrameBufferDescription.h"

namespace slag
{
    FrameBufferDescription &FrameBufferDescription::addColorTarget(Pixels::Format format)
    {
        _colorFormats.push_back(format);
        return *this;
    }

    FrameBufferDescription &FrameBufferDescription::setDepthTarget(Pixels::Format format)
    {
        _depthFormat = format;
        return *this;
    }

    size_t FrameBufferDescription::colorTargetCount()const
    {
        return _colorFormats.size();
    }

    Pixels::Format FrameBufferDescription::colorFormat(size_t index)const
    {
        return _colorFormats[index];
    }

    Pixels::Format FrameBufferDescription::depthFormat()const
    {
        return _depthFormat;
    }
} // slag