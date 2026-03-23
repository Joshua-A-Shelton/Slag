#define VMA_IMPLEMENTATION
#include "VulkanBackend.h"

#include "slag/shader-reflection/spirv/SPIRVShaderReflector.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        void(*SLAG_VULKAN_DEBUG_HANDLER)(const std::string& message, DebugLevel level, int32_t messageID)=nullptr;

        VkBool32 VULKAN_DEBUG_MESSENGER_CALLBACK(VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
                                                VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
                                                const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
                                                void*                                            pUserData)
        {
            if(SLAG_VULKAN_DEBUG_HANDLER!= nullptr)
            {
                DebugLevel level = DebugLevel::INFO;
                if(messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
                {
                    level = DebugLevel::ERROR;
                }
                else if(messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
                {
                    level = DebugLevel::WARNING;
                }
                std::string message = pCallbackData->pMessage;
                SLAG_VULKAN_DEBUG_HANDLER(message,level,pCallbackData->messageIdNumber);
            }
            return VK_FALSE;
        }

        NativeFormat VULKAN_NATIVE_FORMATS[]
        {
            {VK_FORMAT_UNDEFINED, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R32G32B32A32_SFLOAT,  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R32G32B32A32_UINT,  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R32G32B32A32_SINT, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R32G32B32_SFLOAT,  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R32G32B32_UINT,  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R32G32B32_SINT,  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R16G16B16A16_SFLOAT,  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R16G16B16A16_UNORM,  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R16G16B16A16_UINT,  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R16G16B16A16_SNORM,  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R16G16B16A16_SINT,  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R32G32_SFLOAT,  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R32G32_UINT,  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R32G32_SINT,  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_A2B10G10R10_UINT_PACK32, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_B10G11R11_UFLOAT_PACK32, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R8G8B8A8_UNORM,  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R8G8B8A8_SRGB, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R8G8B8A8_UINT, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R8G8B8A8_SNORM, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R8G8B8A8_SINT, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R16G16_SFLOAT, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R16G16_UNORM, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R16G16_UINT, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R16G16_SNORM, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R16G16_SINT, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_D32_SFLOAT, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R32_SFLOAT, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R32_UINT, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R32_SINT, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_D24_UNORM_S8_UINT,  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R8G8_UNORM, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R8G8_UINT, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R8G8_SNORM, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R8G8_SINT, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R16_SFLOAT, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_D16_UNORM, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R16_UNORM, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R16_UINT, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R16_SNORM, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R16_SINT, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R8_UNORM, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R8_UINT, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R8_SNORM, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R8_SINT, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R8_UNORM, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_R},
            {VK_FORMAT_E5B9G9R9_UFLOAT_PACK32, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_B8G8R8G8_422_UNORM, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_G8B8G8R8_422_UNORM, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_BC1_RGBA_UNORM_BLOCK, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_BC1_RGBA_SRGB_BLOCK, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_BC2_UNORM_BLOCK, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_BC2_SRGB_BLOCK, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_BC3_UNORM_BLOCK, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_BC3_SRGB_BLOCK, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_BC4_UNORM_BLOCK, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_BC4_SNORM_BLOCK, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_BC5_UNORM_BLOCK, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_BC5_SNORM_BLOCK, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R5G6B5_UNORM_PACK16, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_A1R5G5B5_UNORM_PACK16, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_B8G8R8A8_UNORM, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_B8G8R8A8_UNORM, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ONE},
            {VK_FORMAT_B8G8R8A8_SRGB, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_B8G8R8A8_SRGB, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ONE},
            {VK_FORMAT_BC6H_UFLOAT_BLOCK, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_BC6H_SFLOAT_BLOCK, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_BC7_UNORM_BLOCK, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_BC7_SRGB_BLOCK, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_R8G8B8A8_UNORM, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_A},
            {VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_G8B8G8R8_422_UNORM,  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_FORMAT_A4R4G4B4_UNORM_PACK16,  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY}
        };

        VulkanBackend::VulkanBackend()
        {
        }

        VulkanBackend::~VulkanBackend()
        {
            _graphicsCards.clear();
            if (_debugMessenger != nullptr)
            {
                vkb::destroy_debug_utils_messenger(_instance,_debugMessenger);
            }
            if (_instance.instance)
            {
                vkDestroyInstance(_instance.instance, nullptr);
            }
            SLAG_VULKAN_DEBUG_HANDLER = nullptr;
        }

        BackendAPI VulkanBackend::api()const
        {
            return BackendAPI::VULKAN;
        }

        uint32_t VulkanBackend::graphicsCardCount()const
        {
            return _graphicsCards.size();
        }

        GraphicsCard* VulkanBackend::graphicsCard(uint32_t index)
        {
            return &_graphicsCards[index];
        }

        uint32_t VulkanBackend::supportedShaderLanguageCount() const
        {
            return 1;
        }

        ShaderLanguage VulkanBackend::supportedShaderLanguage(uint32_t index) const
        {
            return ShaderLanguage::SPIRV;
        }

        VkBufferUsageFlagBits2 VulkanBackend::nativeBufferUsage(BufferMemoryType access)
        {
            VkBufferUsageFlagBits2 bufferUsage = VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT_KHR;
            switch (access)
            {
            case BufferMemoryType::GENERAL:
                bufferUsage |= VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_STORAGE_TEXEL_BUFFER_BIT;
               break;
            case BufferMemoryType::UNIFORM:
                bufferUsage |= VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_2_UNIFORM_TEXEL_BUFFER_BIT;
                break;
            }

            return bufferUsage;
        }

        NativeFormat VulkanBackend::nativeFormat(PixelFormat format)
        {
            return VULKAN_NATIVE_FORMATS[uint32_t(format)];
        }

        VkImageUsageFlags VulkanBackend::nativeTextureUsage(TextureUsageFlags flags)
        {
            VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT ;

            if (static_cast<bool>(flags & TextureUsageFlags::SAMPLED))
            {
                usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
            }
            if (static_cast<bool>(flags & TextureUsageFlags::UNORDERED_ACCESS))
            {
                usage |= VK_IMAGE_USAGE_STORAGE_BIT;
            }
            if (static_cast<bool>(flags & TextureUsageFlags::COLOR_TARGET))
            {
                usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            }
            if (static_cast<bool>(flags & TextureUsageFlags::DEPTH_STENCIL_TARGET))
            {
                usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            }
            return usage;
        }


        VkAccessFlags2 VulkanBackend::nativeMemoryCaches(MemoryCaches caches)
        {
            VkAccessFlags2 flags = VK_ACCESS_2_NONE;

            if (static_cast<bool>(caches & MemoryCaches::INDIRECT_COMMAND_READ))
            {
                flags |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
            }
            if (static_cast<bool>(caches & MemoryCaches::INDEX_READ))
            {
                flags |= VK_ACCESS_2_INDEX_READ_BIT;
            }
            if (static_cast<bool>(caches & MemoryCaches::VERTEX_ATTRIBUTE_READ))
            {
                flags |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
            }
            if (static_cast<bool>(caches & MemoryCaches::UNIFORM_READ))
            {
                flags |= VK_ACCESS_2_UNIFORM_READ_BIT;
            }
            if (static_cast<bool>(caches & MemoryCaches::COLOR_TARGET))
            {
                flags |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            }
            if (static_cast<bool>(caches & MemoryCaches::DEPTH_TARGET_READ))
            {
                flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            }
            if (static_cast<bool>(caches & MemoryCaches::DEPTH_TARGET_WRITE))
            {
                flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            }
            if (static_cast<bool>(caches & MemoryCaches::SHADER_SAMPLED_READ))
            {
                flags |= VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
            }
            if (static_cast<bool>(caches & MemoryCaches::SHADER_UNORDERED_ACCESS))
            {
                flags |= VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
            }
            if (static_cast<bool>(caches & MemoryCaches::BLIT_READ))
            {
                flags |= VK_ACCESS_2_TRANSFER_READ_BIT;
            }
            if (static_cast<bool>(caches & MemoryCaches::BLIT_WRITE))
            {
                flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
            }
            if (static_cast<bool>(caches & MemoryCaches::COPY_READ))
            {
                flags |= VK_ACCESS_2_TRANSFER_READ_BIT;
            }
            if (static_cast<bool>(caches & MemoryCaches::COPY_WRITE))
            {
                flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
            }
            if (static_cast<bool>(caches & MemoryCaches::RESOLVE_READ))
            {
                flags |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
            }
            if (static_cast<bool>(caches & MemoryCaches::RESOLVE_WRITE))
            {
                flags |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            }
            if (static_cast<bool>(caches & MemoryCaches::CLEAR))
            {
                flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
            }
            return flags;
        }

        VkPipelineStageFlags2 VulkanBackend::nativePipelineStages(SyncStages stages)
        {
            VkPipelineStageFlags2 flags = VK_PIPELINE_STAGE_2_NONE;

            if (static_cast<bool>(stages & SyncStages::ALL))
            {
                flags |= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            }
            if (static_cast<bool>(stages & SyncStages::ALL_GRAPHICS))
            {
                flags |= VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
            }
            if (static_cast<bool>(stages & SyncStages::INDEX_INPUT))
            {
                flags |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
            }
            if (static_cast<bool>(stages & SyncStages::VERTEX_SHADER))
            {
                flags |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
            }
            if (static_cast<bool>(stages & SyncStages::FRAGMENT_SHADER))
            {
                flags |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            }
            if (static_cast<bool>(stages & SyncStages::DEPTH_STENCIL_TARGET_OUTPUT))
            {
                flags |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT_KHR;
            }
            if (static_cast<bool>(stages & SyncStages::COLOR_TARGET_OUTPUT))
            {
                flags |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
            if (static_cast<bool>(stages & SyncStages::COMPUTE_SHADER))
            {
                flags |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            }
            if (static_cast<bool>(stages & SyncStages::RAYTRACING_SHADER))
            {
                flags |= VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
            }
            if (static_cast<bool>(stages & SyncStages::COPY))
            {
                flags |= VK_PIPELINE_STAGE_2_COPY_BIT_KHR;
            }
            if (static_cast<bool>(stages & SyncStages::RESOLVE))
            {
                flags |= VK_PIPELINE_STAGE_2_RESOLVE_BIT;
            }
            if (static_cast<bool>(stages & SyncStages::EXECUTE_INDIRECT))
            {
                flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
            }
            if (static_cast<bool>(stages & SyncStages::CLEAR))
            {
                flags |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            }
            if (static_cast<bool>(stages & SyncStages::VIDEO_DECODE))
            {
                flags |= VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR;
            }
            if (static_cast<bool>(stages & SyncStages::VIDEO_ENCODE))
            {
                flags |= VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR;
            }
            if (static_cast<bool>(stages & SyncStages::BUILD_ACCELERATION_STRUCTURE))
            {
                flags |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
            }
            if (static_cast<bool>(stages & SyncStages::COPY_ACCELERATION_STRUCTURE))
            {
                flags |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR;
            }
            return flags;
        }

        SlagInitializationResult VulkanBackend::initializeBackend(const InitializationData& initializationData)
        {
            if (initializationData.debugHandler)
            {
                SLAG_VULKAN_DEBUG_HANDLER = initializationData.debugHandler;
            }

            vkb::InstanceBuilder builder;
            auto inst = builder.set_app_name("Slag Application")
                .request_validation_layers(initializationData.debugHandler)
                .set_debug_callback(VULKAN_DEBUG_MESSENGER_CALLBACK)
                .require_api_version(1,4,341)
                .enable_extension("VK_KHR_get_surface_capabilities2")
                .enable_extension("VK_EXT_surface_maintenance1")
                .build();
            if (!inst.has_value())
            {
                return SlagInitializationResult::INSUFFICIENT_CAPABILITIES;
            }
            _debugMessenger = inst->debug_messenger;
            _instance = inst.value();


            VkPhysicalDeviceFeatures basicFeatures{};
            basicFeatures.fillModeNonSolid = true;
            basicFeatures.wideLines = true;
            basicFeatures.depthClamp = true;
            basicFeatures.sampleRateShading = true;
            basicFeatures.alphaToOne = true;
            basicFeatures.logicOp = true;
            basicFeatures.fragmentStoresAndAtomics = true;
            basicFeatures.shaderStorageImageMultisample = true;

            VkPhysicalDeviceSamplerYcbcrConversionFeatures ycbcrFeatures{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES};
            ycbcrFeatures.samplerYcbcrConversion = true;

            VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR shaderDerivativesFeatures{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_KHR};
            shaderDerivativesFeatures.pNext = &ycbcrFeatures;
            shaderDerivativesFeatures.computeDerivativeGroupLinear = true;
            shaderDerivativesFeatures.computeDerivativeGroupQuads = true;

            VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchainFeatures{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT};
            swapchainFeatures.pNext = &shaderDerivativesFeatures;
            swapchainFeatures.swapchainMaintenance1 = true;

            VkPhysicalDeviceVulkan12Features features1_2{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
            features1_2.bufferDeviceAddress = true;
            features1_2.descriptorIndexing = true;
            features1_2.timelineSemaphore = true;
            features1_2.drawIndirectCount = true;
            features1_2.shaderInt8 = true;
            features1_2.shaderFloat16 = true;
            features1_2.pNext = &swapchainFeatures;


            VkPhysicalDeviceVulkan13Features features1_3{.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
            features1_3.dynamicRendering = true;
            features1_3.synchronization2 = true;

            vkb::PhysicalDeviceSelector selector{_instance};
            auto physicalDevices = selector.set_minimum_version(1,4)
                .set_required_features_13(features1_3)
                .set_required_features_12(features1_2)
                .set_required_features(basicFeatures)
                .add_required_extension("VK_EXT_swapchain_maintenance1")
                .add_required_extension("VK_EXT_custom_border_color")
                .add_required_extension("VK_KHR_compute_shader_derivatives")
                .defer_surface_initialization()
                .select_devices();

            if (!physicalDevices.has_value())
            {
                return SlagInitializationResult::NO_GRAPHICS_CARDS;
            }
            _graphicsCards.reserve(physicalDevices.value().size());
            for (auto& physicalDevice : physicalDevices.value())
            {
                vkb::DeviceBuilder deviceBuilder{physicalDevice};
                auto device = deviceBuilder.build();
                if (device.has_value())
                {
                    _graphicsCards.emplace_back(_instance.instance,device.value());
                }
            }
            if (_graphicsCards.size()==0)
            {
                return SlagInitializationResult::NO_GRAPHICS_CARDS;
            }
            return SlagInitializationResult::SUCCESS;

        }
    } // vulkan
} // slag
