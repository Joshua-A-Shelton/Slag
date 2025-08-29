
#include "Pixels.h"
#include <array>

#include "slag/utilities/SLAG_ASSERT.h"


namespace slag
{
    std::array<uint32_t,(uint32_t)Pixels::Format::PIXELS_FORMAT_MAX> PIXEL_SIZES
    {
#define DEFINITION(SlagName, DxName, VulkanName, VkImageAspectFlags, VkComponentSwizzle_r, VkComponentSwizzle_g, VkComponentSwizzle_b, VkComponentSwizzle_a, totalBits, aspects) totalBits/8,
        SLAG_TEXTURE_FORMAT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
    };

    std::array<Pixels::AspectFlags,(uint32_t)Pixels::Format::PIXELS_FORMAT_MAX> PIXEL_ASPECTS
    {
#define DEFINITION(SlagName, DxName, VulkanName, VkImageAspectFlags, VkComponentSwizzle_r, VkComponentSwizzle_g, VkComponentSwizzle_b, VkComponentSwizzle_a, totalBits, aspects) Pixels::AspectFlags::aspects,
        SLAG_TEXTURE_FORMAT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
    };

    uint32_t Pixels::size(Format format)
    {
        SLAG_ASSERT(format < Pixels::Format::PIXELS_FORMAT_MAX && (uint32_t)format >= 0);
        return PIXEL_SIZES[(uint32_t)format];
    }

    Pixels::AspectFlags Pixels::aspectFlags(Format format)
    {
        SLAG_ASSERT(format < Pixels::Format::PIXELS_FORMAT_MAX && (uint32_t)format >= 0);
        return PIXEL_ASPECTS[(uint32_t)format];
    }

    bool Pixels::isValidAspectFlags(AspectFlags aspectFlags)
    {
        if ((bool)(aspectFlags & Pixels::AspectFlags::COLOR))
        {
            if ((bool)(aspectFlags & Pixels::AspectFlags::DEPTH) || (bool)(aspectFlags & Pixels::AspectFlags::STENCIL))
            {
                return false;
            }
            return true;
        }
        if ((bool)(aspectFlags & Pixels::AspectFlags::STENCIL))
        {
            if ((bool)(aspectFlags & Pixels::AspectFlags::DEPTH))
            {
                return true;
            }
            return false;
        }
        return true;
    }
}
