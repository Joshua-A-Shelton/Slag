#include "slag/core/Texture.h"
#include "slag/core/Texture.h"

#include <gtest/gtest.h>
#include <slag/Slag.h>

#include "slag/core/PixelFormatProperties.h"
#include "slag/core/Pixels.h"
#include "slag/core/Pixels.h"

using namespace slag;

TEST(Texture, CreateWithData)
{
    GTEST_FAIL();
}
TEST(Texture, ChromaFormatUsage)
{
    GTEST_FAIL();
}

#define DEFINITION(SlagName, DxName, VulkanName, VkImageAspectFlags, VkComponentSwizzle_r, VkComponentSwizzle_g, VkComponentSwizzle_b, VkComponentSwizzle_a, totalBits, Aspects)\
    TEST(Texture, SlagName)\
    {\
        auto properties = Pixels::formatProperties(Pixels::Format::SlagName);\
        if(properties.tiling == PixelFormatProperties::Tiling::UNSUPPORTED)\
        {\
            GTEST_SKIP();\
        }\
        auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::SlagName,Texture::Type::TEXTURE_2D,properties.validUsageFlags,64,64,1,1));\
    }\

SLAG_TEXTURE_FORMAT_DEFINTITIONS(DEFINITION)
#undef DEFINITION