#include "VulkanLib.h"
#include "VkBootstrap.h"
#include "VulkanSemaphore.h"
#include "VulkanCommandBuffer.h"
#include "VulkanQueue.h"

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

            vkb::PhysicalDeviceSelector selector{inst.value()};
            auto physicalDevice = selector.set_minimum_version(1,3)
                                          .set_required_features_13(features1_3)
                                          .set_required_features_12(features1_2)
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
            return new VulkanLib(instance,debugMessenger,card);;

        }

        void VulkanLib::cleanup(lib::BackEndLib* library)
        {
            if(get())
            {
                delete library;
            }
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

        Texture* VulkanLib::newTexture(GpuQueue* queue, void* data, Pixels::Format dataFormat, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Usage usage,
                                       Texture::Layout initializedLayout)
        {
            if(dataFormat == textureFormat)
            {
            }
            else
            {

            }
            throw std::runtime_error("implement");
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

    } // vulkan
} // slag