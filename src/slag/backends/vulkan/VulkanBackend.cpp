#define VMA_IMPLEMENTATION
#include "VulkanBackend.h"

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
