#ifndef SLAG_TEXTURE_H
#define SLAG_TEXTURE_H

//slagname, vulkanName, DX12BarrierLayout, DX12ResourceState
#define TEXTURE_LAYOUT_DEFINTITIONS(DEFINITION) \
DEFINITION(UNDEFINED,VK_IMAGE_LAYOUT_UNDEFINED,D3D12_BARRIER_LAYOUT_UNDEFINED,D3D12_RESOURCE_STATE_COMMON) \
DEFINITION(RENDER_TARGET,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,D3D12_BARRIER_LAYOUT_RENDER_TARGET,D3D12_RESOURCE_STATE_RENDER_TARGET ) \
DEFINITION(GENERAL,VK_IMAGE_LAYOUT_GENERAL,D3D12_BARRIER_LAYOUT_COMMON,D3D12_RESOURCE_STATE_COMMON)        \
DEFINITION(UNORDERED,VK_IMAGE_LAYOUT_GENERAL,D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_UNORDERED_ACCESS)        \
DEFINITION(DEPTH_READ,VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ,D3D12_RESOURCE_STATE_DEPTH_READ ) \
DEFINITION(DEPTH_WRITE,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE,D3D12_RESOURCE_STATE_DEPTH_WRITE ) \
DEFINITION(SHADER_RESOURCE,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,D3D12_BARRIER_LAYOUT_SHADER_RESOURCE,D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE) \
DEFINITION(TRANSFER_DESTINATION,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,D3D12_BARRIER_LAYOUT_COPY_DEST,D3D12_RESOURCE_STATE_COPY_DEST) \
DEFINITION(TRANSFER_SOURCE,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,D3D12_BARRIER_LAYOUT_COPY_SOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE) \
DEFINITION(RESOLVE_DESTINATION,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,D3D12_BARRIER_LAYOUT_COPY_DEST,D3D12_RESOURCE_STATE_RESOLVE_DEST) \
DEFINITION(RESOLVE_SOURCE,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,D3D12_BARRIER_LAYOUT_COPY_SOURCE,D3D12_RESOURCE_STATE_RESOLVE_SOURCE) \
DEFINITION(PRESENT,VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,D3D12_BARRIER_LAYOUT_PRESENT,D3D12_RESOURCE_STATE_PRESENT ) \

//Technically, other options are defined, but I'm sticking with these for now
#define TEXTURE_USAGE_DEFINITIONS(DEFINITION) \
DEFINITION(SAMPLED_IMAGE,VK_IMAGE_USAGE_SAMPLED_BIT,0) \
DEFINITION(INPUT_ATTACHMENT,VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,0)\
DEFINITION(STORAGE,VK_IMAGE_USAGE_STORAGE_BIT,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)  \
DEFINITION(RENDER_TARGET_ATTACHMENT,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) \
DEFINITION(DEPTH_STENCIL_ATTACHMENT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) \


#include "Pixel.h"
#include "Color.h"
#include "Sampler.h"
#include <filesystem>

namespace slag
{
    class TextureUsage
    {
    private:
        int _value;
        TextureUsage(int val){_value=val;}
    public:
        friend class TextureUsageFlags;

        TextureUsage operator| (TextureUsage b) const
        {
            return TextureUsage(_value | b._value);
        }

        TextureUsage& operator |=(TextureUsage b)
        {
            _value = _value|b._value;
            return *this;
        }

        TextureUsage operator&(TextureUsage b) const
        {
            return TextureUsage(_value & b._value);
        }

        TextureUsage& operator&=(TextureUsage b)
        {
            _value = _value&b._value;
            return *this;
        }

        TextureUsage operator~() const
        {
            return TextureUsage(~_value);
        }
    };

    class TextureUsageFlags
    {
    private:
#define DEFINITION(slagName, vulkanName, dirextXName) inline static TextureUsage _##slagName = TextureUsage(0);
        TEXTURE_USAGE_DEFINITIONS(DEFINITION)
#undef DEFINITION
    public:
#define DEFINITION(slagName, vulkanName, dirextXName) inline static const TextureUsage& slagName = _##slagName; /***Only use this if you *REALLY* know what you're doing, will override underlying library value for flag*/ static void set##slagName##Value(int value){_##slagName._value = value;}
        TEXTURE_USAGE_DEFINITIONS(DEFINITION)
#undef DEFINITION
    };

    class Texture
    {
    public:
        enum Layout
        {
#define DEFINITION(slagName, vulkanName, directXName,directXResourceName) slagName,
            TEXTURE_LAYOUT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
        };

        enum Type
        {
            TEXTURE_1D,
            TEXTURE_2D,
            TEXTURE_3D,
            CUBE_MAP
        };

        virtual ~Texture()=default;

        /**
         * What the texture type is
         * @return
         */
        virtual Type type()=0;
        /**
         * The width of the texture
         * @return
         */
        virtual uint32_t width()=0;
        /**
         * the height of the texture, (always 1 for 1D textures)
         * @return
         */
        virtual uint32_t height()=0;
        /**
         * number of elements in the arrary (1D and 2D textures), or number of depth slices (3D texture), (always 6 for cubemap)
         * @return
         */
        virtual uint32_t layers()=0;
        /**
         * the number of mip levels (lower res LOD images) per texture
         * @return
         */
        virtual uint32_t mipLevels()=0;
        /**
         * MSAA Sample Count
         * @return
         */
        virtual uint8_t sampleCount()=0;


        /**
         * Raw dogging it texture. Give it the data it needs, and it assumes it's all correct. should be *fast*, but requires the user of the API to be doing the homework
         * @param data
         * @param dataSize
         * @param dataFormat
         * @param width
         * @param height
         * @param mipLevels
         * @param layers
         * @param sampleCount
         * @param usage
         * @param initializedLayout
         * @return
         */
        static Texture* newTexture(const std::filesystem::path& imagePath, uint32_t mipLevels, TextureUsage usage, bool sRGB, Layout initializedLayout);
        static Texture* newTexture(Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layers, uint8_t sampleCount, TextureUsage usage);
    };

} // slag

#endif //CRUCIBLEEDITOR_TEXTURE_H
