#ifndef SLAG_EXTENSIONS_H
#define SLAG_EXTENSIONS_H
#include <vulkan/vulkan.h>

namespace slag
{
    namespace vulkan
    {
        class Extensions
        {
        public:
            inline static PFN_vkCopyImageToImageEXT vkCopyImageToImageEXT=nullptr;
            static void mapExtensions(VkDevice fromDevice);
        };

    } // vulkan
} // slag

#endif //SLAG_EXTENSIONS_H
