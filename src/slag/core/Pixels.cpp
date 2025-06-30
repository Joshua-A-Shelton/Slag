
#include "Pixels.h"
#include <array>

#include "slag/utilities/SLAG_ASSERT.h"


namespace slag
{
    std::array<uint32_t,Pixels::PIXELS_FORMAT_MAX> PIXEL_SIZES
    {
#define DEFINITION(SlagName, DxName, VulkanName, VkImageAspectFlags, VkComponentSwizzle_r, VkComponentSwizzle_g, VkComponentSwizzle_b, VkComponentSwizzle_a, totalBits) totalBits/8,
        TEXTURE_FORMAT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
    };

    uint32_t Pixels::size(Format format)
    {
        SLAG_ASSERT(format < Pixels::PIXELS_FORMAT_MAX && format >= 0);
        return PIXEL_SIZES[format];
    }
}
