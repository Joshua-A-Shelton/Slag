#include "VulkanLib.h"
#include "VkBootstrap.h"
#include "VulkanSemaphore.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanSwapchain.h"
#include "VulkanQueue.h"
#include "VulkanTexture.h"
#include "Extensions.h"
#include "VulkanSampler.h"
#include "VulkanDescriptorGroup.h"

namespace slag
{
    namespace vulkan
    {

        bool _includeVulkanLayers = true;

        VulkanLib* VulkanLib::initialize()
        {
#if NDEBUG
            _includeVulkanLayers = false;
#endif
            vkb::InstanceBuilder builder;
            auto inst = builder.set_app_name("Slag Application")
                               .request_validation_layers(_includeVulkanLayers)
                               .use_default_debug_messenger()
                               .require_api_version(1,3,0)
                               .build();

            if(!inst.has_value())
            {
                return nullptr;
            }
            auto instance = inst->instance;
            auto debugMessenger = inst->debug_messenger;

            VkPhysicalDeviceVulkan13Features features1_3{.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
            features1_3.dynamicRendering = true;
            features1_3.synchronization2 = true;


            VkPhysicalDeviceVulkan12Features features1_2{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
            features1_2.bufferDeviceAddress = true;
            features1_2.descriptorIndexing = true;
            features1_2.timelineSemaphore = true;

            vkb::PhysicalDeviceSelector selector{inst.value()};
            auto physicalDevice = selector.set_minimum_version(1,3)
                                          .set_required_features_13(features1_3)
                                          .set_required_features_12(features1_2)
                                          .add_required_extension("VK_EXT_swapchain_maintenance1")
                                          .add_required_extension("VK_EXT_host_image_copy")
                                          .add_required_extension("VK_EXT_custom_border_color")
                                          .defer_surface_initialization()
                                          .select();
            if(!physicalDevice.has_value())
            {
                return nullptr;
            }
            vkb::DeviceBuilder deviceBuilder{physicalDevice.value()};
            auto device = deviceBuilder.build();
            if(!device.has_value())
            {
                return nullptr;
            }
            auto card = new VulkanGraphicsCard(instance,device.value());
            Extensions::mapExtensions(card->device());

            mapFlags();

            return new VulkanLib(instance,debugMessenger,card);

        }

        void VulkanLib::cleanup(lib::BackEndLib* library)
        {
            if(get())
            {
                delete library;
            }
        }

        void VulkanLib::mapFlags()
        {
#define DEFINITION(slagName, vulkanName, directXName) TextureUsageFlags::set##slagName##Value(static_cast<int>(vulkanName));
            TEXTURE_USAGE_DEFINITIONS(DEFINITION)
#undef DEFINITION

#define DEFINITION(slagName, vulkanName, directXName) BarrierAccessFlags::set##slagName##Value(static_cast<int>(vulkanName));
            MEMORY_BARRIER_ACCESS_DEFINTITIONS(DEFINITION)
#undef DEFINITION

#define DEFINITION(slagName, vulkanName, directXName) PipelineStageFlags::set##slagName##Value(static_cast<int>(vulkanName));
            MEMORY_PIPELINE_STAGE_DEFINITIONS(DEFINITION)
#undef DEFINITION

#define DEFINITION(slagName, vulkanName, directXName) ShaderStageFlags::set##slagName##Value(static_cast<int>(vulkanName));
            SHADER_STAGE_DEFINTITIONS(DEFINITION)
#undef DEFINITION

        }

        VkInstance VulkanLib::instance()
        {
            return _instance;
        }

        VulkanLib* VulkanLib::get()
        {
            return dynamic_cast<VulkanLib*>(lib::BackEndLib::get());
        }

        VulkanGraphicsCard* VulkanLib::card()
        {
            return get()->vulkanGraphicsCard();
        }

        VulkanizedFormat VulkanLib::format(Pixels::Format format)
        {
            switch (format)
            {
#define DEFINITION(SlagName, DxName, VulkanName, VkImageAspectFlags, VkComponentSwizzle_r, VkComponentSwizzle_g, VkComponentSwizzle_b, VkComponentSwizzle_a, totalBits) case Pixels::SlagName: return {.format = VulkanName, .mapping = {.r = VkComponentSwizzle_r, .g = VkComponentSwizzle_g, .b = VkComponentSwizzle_b, .a = VkComponentSwizzle_a} };
                TEXTURE_FORMAT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }

            return VulkanizedFormat{};
        }

        VkImageLayout VulkanLib::layout(Texture::Layout layout)
        {
            switch(layout)
            {
#define DEFINITION(slagName, vulkanName, directXName, directXResourceName) case Texture::slagName: return vulkanName;
                TEXTURE_LAYOUT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return VK_IMAGE_LAYOUT_UNDEFINED;
        }

        VkImageType VulkanLib::imageType(Texture::Type imageType)
        {
            switch (imageType)
            {
                case Texture::Type::TEXTURE_1D:
                    return VK_IMAGE_TYPE_1D;
                case Texture::Type::TEXTURE_2D:
                case Texture::Type::CUBE_MAP:
                    return VK_IMAGE_TYPE_2D;
                case Texture::Type::TEXTURE_3D:
                    return VK_IMAGE_TYPE_3D;
            }
            return VK_IMAGE_TYPE_2D;
        }

        VkImageViewType VulkanLib::viewType(Texture::Type textureType, size_t layerCount)
        {
            if(layerCount > 1)
            {
                switch (textureType)
                {
                    case Texture::TEXTURE_1D:
                        return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
                    case Texture::TEXTURE_2D:
                        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                    case Texture::TEXTURE_3D:
                        return VK_IMAGE_VIEW_TYPE_3D;
                    case Texture::CUBE_MAP:
                        if(layerCount != 6)
                        {
                            throw std::runtime_error("cannot have cubemap with non 6 sided layer count");
                        }
                        return VK_IMAGE_VIEW_TYPE_CUBE;
                }
            }
            else
            {
                switch (textureType)
                {
                    case Texture::TEXTURE_1D:
                        return VK_IMAGE_VIEW_TYPE_1D;
                    case Texture::TEXTURE_2D:
                        return VK_IMAGE_VIEW_TYPE_2D;
                    case Texture::TEXTURE_3D:
                        return VK_IMAGE_VIEW_TYPE_3D;
                    case Texture::CUBE_MAP:
                        throw std::runtime_error("cannot have cubemap with non 6 sided layer count");
                }
            }
            return VK_IMAGE_VIEW_TYPE_2D;
        }

        VkFilter VulkanLib::filter(Sampler::Filter filter)
        {
            switch (filter)
            {
#define DEFINITION(slagName, vulkanName) case Sampler::slagName: return vulkanName;
                SAMPLER_FILTER_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return VK_FILTER_LINEAR;
        }

        VkSamplerMipmapMode VulkanLib::mipMapMode(Sampler::Filter filter)
        {
            switch (filter)
            {
                case Sampler::Filter::NEAREST:
                    return VK_SAMPLER_MIPMAP_MODE_NEAREST;
                case Sampler::Filter::LINEAR:
                    return VK_SAMPLER_MIPMAP_MODE_LINEAR;
            }
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        }

        VkSamplerAddressMode VulkanLib::addressMode(Sampler::AddressMode addressMode)
        {
            switch (addressMode)
            {
#define DEFINITION(slagName, vulkanName, dx12Name) case Sampler::slagName: return vulkanName;
                SAMPLER_ADDRESS_MODES_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }

        VkCompareOp VulkanLib::compareOp(Sampler::ComparisonFunction comparisonFunction)
        {
            switch (comparisonFunction)
            {
#define DEFINITION(slagName, vulkanName, dx12Name) case Sampler::slagName: return vulkanName;
                SAMPLER_COMPARISON_FUNCTION(DEFINITION)
#undef DEFINITION
            }
            return VK_COMPARE_OP_NEVER;
        }

        VkDescriptorType VulkanLib::descriptorType(slag::Descriptor::DescriptorType descriptorType)
        {
            switch (descriptorType)
            {
                case slag::Descriptor::DescriptorType::SAMPLER:
                    return  VK_DESCRIPTOR_TYPE_SAMPLER;
                case slag::Descriptor::DescriptorType::SAMPLED_TEXTURE:
                    return  VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                case slag::Descriptor::DescriptorType::SAMPLER_AND_TEXTURE:
                    return  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                case slag::Descriptor::DescriptorType::STORAGE_TEXTURE:
                    return  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                case slag::Descriptor::DescriptorType::UNIFORM_TEXEL_BUFFER:
                    return  VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                case slag::Descriptor::DescriptorType::STORAGE_TEXEL_BUFFER:
                    return  VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                case slag::Descriptor::DescriptorType::UNIFORM_BUFFER:
                    return  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                case slag::Descriptor::DescriptorType::STORAGE_BUFFER:
                    return  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
                case slag::Descriptor::DescriptorType::INPUT_ATTACHMENT:
                    return  VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            }
            throw std::runtime_error("unable to convert descriptorType");
        }

        VulkanLib::VulkanLib(VkInstance instance, VkDebugUtilsMessengerEXT messenger, VulkanGraphicsCard* card)
        {
            _instance = instance;
            _debugMessenger = messenger;
            _graphicsCard = card;
        }

        VulkanLib::~VulkanLib()
        {
            if(_debugMessenger)
            {
                vkb::destroy_debug_utils_messenger(_instance,_debugMessenger);
            }
            if(graphicsCard())
            {
                delete _graphicsCard;
            }
            if(instance())
            {
                vkDestroyInstance(_instance, nullptr);
            }
        }

        BackEnd VulkanLib::identifier()
        {
            return Vulkan;
        }

        GraphicsCard* VulkanLib::graphicsCard()
        {
            return _graphicsCard;
        }
        VulkanGraphicsCard* VulkanLib::vulkanGraphicsCard()
        {
            return _graphicsCard;
        }

        Semaphore* VulkanLib::newSemaphore(uint64_t startingValue)
        {
            return new VulkanSemaphore(startingValue);
        }

        void VulkanLib::waitFor(SemaphoreValue* values, size_t count)
        {
            VulkanSemaphore::waitFor(values,count);
        }

        Swapchain* VulkanLib::newSwapchain(PlatformData platformData, uint32_t width, uint32_t height, uint8_t backBuffers, Swapchain::PresentMode mode, Pixels::Format imageFormat)
        {
            return new VulkanSwapchain(platformData,width,height,backBuffers,mode, format(imageFormat));
        }

        Texture* VulkanLib::newTexture(void* data, size_t dataSize, Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layers, uint8_t sampleCount, TextureUsage usage, Texture::Layout initializedLayout)
        {
            return new VulkanTexture(data,dataSize,dataFormat,type,width,height,layers,mipLevels,sampleCount,std::bit_cast<VkImageUsageFlags>(usage),initializedLayout, false);
        }

        Texture* VulkanLib::newTexture(void** texelDataArray, size_t texelDataCount, size_t dataSize, Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, TextureUsage usage, Texture::Layout initializedLayout)
        {
            return new VulkanTexture(texelDataArray,texelDataCount,dataSize,dataFormat,type,width,height,mipLevels,std::bit_cast<VkImageUsageFlags>(usage),initializedLayout,false);
        }

        CommandBuffer* VulkanLib::newCommandBuffer(GpuQueue::QueueType acceptsCommands)
        {
            uint32_t family = VulkanLib::_graphicsCard->graphicsQueueFamily();
            switch (acceptsCommands)
            {
                case GpuQueue::Graphics:
                    family = VulkanLib::_graphicsCard->graphicsQueueFamily();
                    break;
                case GpuQueue::Transfer:
                    family = VulkanLib::_graphicsCard->transferQueueFamily();
                    break;
                case GpuQueue::Compute:
                    family = VulkanLib::_graphicsCard->computeQueueFamily();
                    break;

            }
            return new VulkanCommandBuffer(family);
        }

        Buffer* VulkanLib::newBuffer(void* data, size_t dataSize, Buffer::Accessibility accessibility, Buffer::Usage usage)
        {
            VkBufferUsageFlags usageFlags = 0;

            if(usage & Buffer::Usage::VERTEX_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            }
            if(usage & Buffer::Usage::INDEX_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            }
            if(usage & Buffer::Usage::STORAGE_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }
            if(usage & Buffer::Usage::INDIRECT_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            }

            return new VulkanBuffer(data,dataSize,accessibility,usageFlags, false);
        }

        Buffer* VulkanLib::newBuffer(size_t bufferSize, Buffer::Accessibility accessibility, Buffer::Usage usage)
        {
            VkBufferUsageFlags usageFlags = 0;

            if(usage & Buffer::Usage::VERTEX_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            }
            if(usage & Buffer::Usage::INDEX_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            }
            if(usage & Buffer::Usage::STORAGE_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }
            if(usage & Buffer::Usage::INDIRECT_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            }

            return new VulkanBuffer(bufferSize,accessibility,usageFlags, false);
        }

        Sampler* VulkanLib::newSampler(Sampler::Filter minFilter, Sampler::Filter magFilter, Sampler::Filter mipMapFilter, Sampler::AddressMode u, Sampler::AddressMode v, Sampler::AddressMode w, float mipLODBias, bool enableAnisotrophy, uint8_t maxAnisotrophy,Sampler::ComparisonFunction comparisonFunction, Color borderColor, float minLOD, float maxLOD)
        {
            return new VulkanSampler(minFilter,magFilter,mipMapFilter,u,v,w,mipLODBias,enableAnisotrophy,maxAnisotrophy,comparisonFunction,minLOD,maxLOD,borderColor, false);
        }

        DescriptorGroup* VulkanLib::newDescriptorGroup(Descriptor* descriptors, size_t descriptorCount)
        {
            return new VulkanDescriptorGroup(descriptors,descriptorCount);
        }

    } // vulkan
} // slag