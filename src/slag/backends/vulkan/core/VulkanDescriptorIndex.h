#ifndef SLAG_VULKANDESCRIPTORINDEX_H
#define SLAG_VULKANDESCRIPTORINDEX_H
#include <slag/Slag.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanDescriptorIndex: public DescriptorIndex
        {
        public:
            uint32_t binding=0;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANDESCRIPTORINDEX_H
