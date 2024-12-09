#ifndef SLAG_FRAMEBUFFERDESCRIPTION_H
#define SLAG_FRAMEBUFFERDESCRIPTION_H
#include "Pixel.h"
#include <vector>
namespace slag
{
    ///Description for a shader of what it's render targets/depth buffer will be
    class FrameBufferDescription
    {
    public:
        ///Add color texture target format (up to 8) that the shader can render to
        FrameBufferDescription& addColorTarget(Pixels::Format format);
        ///Set the format of the depth/stencil buffer, if one is required (UNKNOWN if none is present)
        FrameBufferDescription& setDepthTarget(Pixels::Format format);
        ///Number of color targets
        size_t colorTargetCount()const;
        ///Retrieve the format of the Color Attachment at the given index
        Pixels::Format colorFormat(size_t index)const;
        ///Retrieve the format of the depth target (UNKNOWN if none is present)
        Pixels::Format depthFormat()const;
    private:
        std::vector<Pixels::Format> _colorFormats;
        Pixels::Format _depthFormat = Pixels::UNDEFINED;
    };

} // slag

#endif //SLAG_FRAMEBUFFERDESCRIPTION_H
