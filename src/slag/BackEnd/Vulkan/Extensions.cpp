#include <cassert>
#include "Extensions.h"

namespace slag
{
    namespace vulkan
    {
        void Extensions::mapExtensions(VkDevice fromDevice)
        {
            //vkCopyImageToImageEXT = reinterpret_cast<PFN_vkCopyImageToImageEXT>(vkGetDeviceProcAddr(fromDevice, "vkCopyImageToImageEXT"));
            //assert(vkCopyImageToImageEXT != nullptr && "unable to get image to image copy extension");
        }
    } // vulkan
} // slag