#ifndef SLAG_FRAMEBUFFERDESCRIPTION_H
#define SLAG_FRAMEBUFFERDESCRIPTION_H
#include <vector>
#include "PixelFormat.h"

namespace slag
{

    class FramebufferDescription
    {
    public:
        FramebufferDescription& addColorTarget(Pixels::PixelFormat format);
        FramebufferDescription& setDepthTarget(Pixels::PixelFormat format);
        size_t colorTargetCount();
        Pixels::PixelFormat colorFormat(size_t index);
        Pixels::PixelFormat depthFormat();
    private:
        std::vector<Pixels::PixelFormat> _colorFormats;
        Pixels::PixelFormat _depthFormat = Pixels::UNDEFINED;
    };

} // slag

#endif //SLAG_FRAMEBUFFERDESCRIPTION_H
