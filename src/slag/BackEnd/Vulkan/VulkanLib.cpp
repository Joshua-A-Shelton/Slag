#include "VulkanLib.h"
#include "VkBootstrap.h"
#include "VulkanSemaphore.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanSwapchain.h"
#include "VulkanQueue.h"
#include "VulkanTexture.h"
#include "Extensions.h"

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
#define DEFINITION(slagName, vulkanName, directXName) case Texture::slagName: return vulkanName;
                TEXTURE_LAYOUT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return VK_IMAGE_LAYOUT_UNDEFINED;
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

        Texture* VulkanLib::newTexture(void* texelData, size_t dataSize, Pixels::Format dataFormat, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, TextureUsage usage, Texture::Layout initializedLayout, bool generateMips)
        {
            return new VulkanTexture(texelData,dataSize, format(dataFormat).format, format(textureFormat),width,height,mipLevels,std::bit_cast<VkImageUsageFlags>(usage),layout(initializedLayout),generateMips,false);
        }

        Texture* VulkanLib::newTexture(CommandBuffer* onBuffer, void* texelData, size_t dataSize, Pixels::Format dataFormat, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, TextureUsage usage, Texture::Layout initializedLayout, bool generateMips)
        {
            return new VulkanTexture(dynamic_cast<VulkanCommandBuffer*>(onBuffer),texelData,dataSize, format(dataFormat).format, format(textureFormat),width,height,mipLevels,std::bit_cast<VkImageUsageFlags>(usage),layout(initializedLayout),generateMips,false);
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

            if(usage & Buffer::Usage::VertexBuffer)
            {
                usageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            }
            if(usage & Buffer::Usage::IndexBuffer)
            {
                usageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            }
            if(usage & Buffer::Usage::Storage)
            {
                usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }
            if(usage & Buffer::Usage::Indirect)
            {
                usageFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            }

            return new VulkanBuffer(data,dataSize,accessibility,usageFlags, false);
        }

        Buffer* VulkanLib::newBuffer(size_t bufferSize, Buffer::Accessibility accessibility, Buffer::Usage usage)
        {
            VkBufferUsageFlags usageFlags = 0;

            if(usage & Buffer::Usage::VertexBuffer)
            {
                usageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            }
            if(usage & Buffer::Usage::IndexBuffer)
            {
                usageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            }
            if(usage & Buffer::Usage::Storage)
            {
                usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }
            if(usage & Buffer::Usage::Indirect)
            {
                usageFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            }

            return new VulkanBuffer(bufferSize,accessibility,usageFlags, false);
        }

    } // vulkan
} // slag