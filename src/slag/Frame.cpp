#include "Frame.h"
#include <xutility>
namespace slag
{
    Frame::Frame(FrameResources* resources)
    {
        this->resources = resources;
    }
    Frame::~Frame()
    {
        if(resources)
        {
            delete resources;
        }
    }

    Frame::Frame(Frame&& from)
    {
        std::swap(resources,from.resources);
    }

    Frame& Frame::operator=(Frame&& from)
    {
        std::swap(resources,from.resources);
        return *this;
    }

    void Frame::move(Frame& from)
    {
        std::swap(resources,from.resources);
    }

} // slag