#ifndef SLAG_FRAMEBUFFERDESCRIPTION_H
#define SLAG_FRAMEBUFFERDESCRIPTION_H
#include "Pixel.h"
#include <vector>
namespace slag
{

    class FrameBufferDescription
    {
    public:
        FrameBufferDescription& addColorTarget(Pixels::Format format);
        FrameBufferDescription& setDepthTarget(Pixels::Format format);
        size_t colorTargetCount()const;
        Pixels::Format colorFormat(size_t index)const;
        Pixels::Format depthFormat()const;
    private:
        std::vector<Pixels::Format> _colorFormats;
        Pixels::Format _depthFormat = Pixels::UNDEFINED;
    };

} // slag

#endif //SLAG_FRAMEBUFFERDESCRIPTION_H
