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
DEFINITION(TRANSFER_DESTINATION,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,D3D12_RESOURCE_STATE_COPY_DEST) \
DEFINITION(TRANSFER_SOURCE,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,D3D12_RESOURCE_STATE_COPY_SOURCE) \
DEFINITION(PRESENT,VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,D3D12_RESOURCE_STATE_PRESENT) \

//Technically, other options are defined, but I'm sticking with these for now
#define TEXTURE_USAGE_DEFINITIONS(DEFINITION) \
DEFINITION(SAMPLED_IMAGE,0b00000001,VK_IMAGE_USAGE_SAMPLED_BIT,0) \
DEFINITION(STORAGE,0b00000010,VK_IMAGE_USAGE_STORAGE_BIT,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)  \
DEFINITION(RENDER_TARGET_ATTACHMENT,0b00000100,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) \
DEFINITION(DEPTH_STENCIL_ATTACHMENT,0b00001000,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) \

#include "Pixel.h"
#include "Color.h"
#include <filesystem>

namespace slag
{

    class Texture
    {
    public:
        enum Usage
        {
#define DEFINITION(slagName, slagValue, vulkanName, dirextXName) slagName = slagValue,
            TEXTURE_USAGE_DEFINITIONS(DEFINITION)
#undef DEFINITION
        };
        enum Layout
        {
#define DEFINITION(slagName, vulkanName, directXName) slagName,
            TEXTURE_LAYOUT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
        };

        virtual ~Texture()=default;

        virtual uint32_t width()=0;
        virtual uint32_t height()=0;
        virtual uint32_t mipLevels()=0;

        static Texture* newTexture(void* data, size_t dataSize, Pixels::Format dataFormat, uint32_t width, uint32_t height, uint32_t mipLevels, Usage usage, Layout initializedLayout, bool generateMipMaps);
        static Texture* newTexture(void* data, size_t dataSize, Pixels::Format dataFormat, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, Usage usage, Layout initializedLayout, bool generateMipMaps);
        static Texture* newTexture(const std::filesystem::path& imagePath, Pixels::Format textureFormat, uint32_t mipLevels, Usage usage, Layout initializedLayout, bool generateMipMaps);
        static Texture* newTexture(Color* colorArray, size_t colorCount, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, Usage usage, Layout initializedLayout, bool generateMipMaps);
    };

} // slag

#endif //CRUCIBLEEDITOR_TEXTURE_H
