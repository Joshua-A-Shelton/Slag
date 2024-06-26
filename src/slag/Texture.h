#ifndef SLAG_TEXTURE_H
#define SLAG_TEXTURE_H

//slagname, vulkanName, DX12Name
#define TEXTURE_LAYOUT_DEFINTITIONS(DEFINITION) \
DEFINITION(UNDEFINED,VK_IMAGE_LAYOUT_UNDEFINED,D3D12_RESOURCE_STATE_COMMON) \
DEFINITION(RENDER_TARGET,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,D3D12_RESOURCE_STATE_RENDER_TARGET) \
DEFINITION(GENERAL,VK_IMAGE_LAYOUT_GENERAL,D3D12_RESOURCE_STATE_UNORDERED_ACCESS) \
DEFINITION(DEPTH_READ,VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,D3D12_RESOURCE_STATE_DEPTH_READ) \
DEFINITION(DEPTH_WRITE,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,D3D12_RESOURCE_STATE_DEPTH_WRITE) \
DEFINITION(SHADER_RESOURCE,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) \
DEFINITION(COPY_DESTINATION,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,D3D12_RESOURCE_STATE_COPY_DEST) \
DEFINITION(COPY_SOURCE,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,D3D12_RESOURCE_STATE_COPY_SOURCE) \
DEFINITION(PRESENT,VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,D3D12_RESOURCE_STATE_PRESENT) \

#include <cstdint>
#include "PixelFormat.h"

namespace slag
{
    class Texture
    {

    public:
        enum Usage
        {
            COLOR = 0x00000001,
            DEPTH = 0x00000002,
            STENCIL = 0x00000004,
            DEPTH_STENCIL = DEPTH | STENCIL
        };
        enum Layout
        {
#define DEFINITION(slagName, vulkanName, directXName) slagName,
            TEXTURE_LAYOUT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
        };

        virtual ~Texture()=default;

        virtual Pixels::PixelFormat format()=0;
        virtual uint32_t mipLevels()=0;
        virtual uint32_t width()=0;
        virtual uint32_t height()=0;
        virtual Usage usage()=0;
        static Texture* create(const char* fileLocation, unsigned int mipLevels=1, bool renderTargetCapable = false);
        static Texture* create(uint32_t width, uint32_t height, Pixels::PixelFormat format, uint32_t mipLevels = 1, bool renderTargetCapable = false);
        static Texture* create(uint32_t width, uint32_t height, Pixels::PixelFormat format, void* pixelData, uint32_t mipLevels = 1, bool renderTargetCapable = false);
    };
}
#endif //SLAG_TEXTURE_H