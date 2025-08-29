#include "Frame.h"

namespace slag
{
    Frame::~Frame()
    {
        if (_resources)
        {
            delete _resources;
        }
    }

    FrameResources* Frame::getFrameResources()
    {
        return _resources;
    }

    Frame::Frame(FrameResources* resources)
    {
        _resources = resources;
    }

    void Frame::move(Frame& from)
    {
        _resources = from._resources;
        from._resources = nullptr;
    }
} // slag
