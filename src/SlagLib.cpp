#include "SlagLib.h"
#include <VkBootstrap.h>
#include <stdexcept>
#include "BackEnd/VulkanBackEnd.h"
#include <vector>
namespace slag
{

    bool usingValidationLayers;
    std::vector<GraphicsCard*> graphicsCards;
    VkDebugUtilsMessengerEXT debugMessenger;
    void initialize(bool includeValidationLayers)
    {
        usingValidationLayers = includeValidationLayers;
        vkb::InstanceBuilder builder;
        auto instance_ret = builder.set_app_name("Slag Application").request_validation_layers(includeValidationLayers).use_default_debug_messenger().build();
        if(!instance_ret)
        {
            throw std::runtime_error("Failed to create vulkan instance");
        }
        vkb::Instance instance = instance_ret.value();
        slag::_vulkanInstance() = instance.instance;
        debugMessenger = instance.debug_messenger;

        vkb::PhysicalDeviceSelector selector{ instance };
        auto devices = selector.set_minimum_version(1,3).defer_surface_initialization().require_dedicated_transfer_queue().select_devices();
        if(!devices)
        {
            throw std::runtime_error("No graphics cards meet the required criteria for rendering");
        }
        for(auto& device : devices.value())
        {
            vkb::DeviceBuilder deviceBuilder{device};
            auto dev_ret = deviceBuilder.build();
            if(dev_ret)
            {
                vkb::Device vkb_device = dev_ret.value();
                graphicsCards.push_back(new GraphicsCard(&vkb_device));
            }
        }

    }

    void destroy()
    {
        for(auto& card : graphicsCards)
        {
            delete card;
        }
        graphicsCards.clear();
        vkb::destroy_debug_utils_messenger(slag::_vulkanInstance(),debugMessenger);
        vkDestroyInstance(slag::_vulkanInstance(), nullptr);
    }



    size_t graphicsCardCount()
    {
        return  graphicsCards.size();
    }

    GraphicsCard *getGraphicsCard(size_t index)
    {
        return graphicsCards.at(index);
    }
}