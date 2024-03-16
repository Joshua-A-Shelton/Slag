#include "VulkanLib.h"
#include "VulkanExtensions.h"

namespace slag
{
    namespace vulkan
    {

        VkDebugUtilsMessengerEXT  _debugMessenger = nullptr;
        VkInstance _instance = nullptr;
        VulkanGraphicsCard* _vulkangraphicsCard = nullptr;

        bool includeVulkanLayers = true;


        bool VulkanLib::initialize()
        {
#if NDEBUG
            includeVulkanLayers = false;
#endif
            vkb::InstanceBuilder instanceBuilder;
            auto instance_ret = instanceBuilder.set_app_name("Crucible Application").request_validation_layers(includeVulkanLayers).use_default_debug_messenger().build();
            if(!instance_ret.has_value())
            {
                return false;
            }
            _instance = instance_ret->instance;
            _debugMessenger = instance_ret->debug_messenger;

            vkb::PhysicalDeviceSelector physicalDeviceSelector{instance_ret.value()};

            VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeaturesKHR
                    {
                            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
                            .dynamicRendering = VK_TRUE

                    };

            vkb::PhysicalDevice physicalDevice = physicalDeviceSelector.set_minimum_version(1,3)
                    .defer_surface_initialization()
                    .add_required_extension("VK_KHR_dynamic_rendering")
                    .add_required_extension("VK_KHR_depth_stencil_resolve")
                    .add_required_extension("VK_KHR_create_renderpass2")
                    .add_required_extension("VK_KHR_multiview")
                    .add_required_extension("VK_KHR_maintenance2")
                    .add_required_extension("VK_EXT_graphics_pipeline_library")
                    .add_required_extension("VK_KHR_pipeline_library")
                    .add_required_extension("VK_KHR_synchronization2")
                    .add_required_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME)
                    .add_required_extension_features(dynamicRenderingFeaturesKHR)
                    .select().value();
            vkb::DeviceBuilder deviceBuilder {physicalDevice};
            vkb::Device vkbDevice = deviceBuilder.build().value();
            _vulkangraphicsCard = new VulkanGraphicsCard(vkbDevice);
            VulkanExtensions::initExtensions(_vulkangraphicsCard);
            return true;
        }

        void VulkanLib::cleanup()
        {
            if(_debugMessenger)
            {
                vkb::destroy_debug_utils_messenger(_instance,_debugMessenger);
            }
            if(graphicsCard())
            {
                delete _vulkangraphicsCard;
            }
            if(instance())
            {
                vkDestroyInstance(_instance, nullptr);
            }
        }

        VkInstance VulkanLib::instance()
        {
            return _instance;
        }

        VulkanGraphicsCard* VulkanLib::graphicsCard()
        {
            return _vulkangraphicsCard;
        }
    } // slag
} // vulkan