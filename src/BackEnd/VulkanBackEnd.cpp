#include "VulkanBackEnd.h"
namespace slag
{
    VkInstance _vkInstance;

    VkInstance& _vulkanInstance()
    {
        return  _vkInstance;
    }
}
