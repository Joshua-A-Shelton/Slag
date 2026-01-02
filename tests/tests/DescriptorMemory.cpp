#include <gtest/gtest.h>
#include <slag/Slag.h>

using namespace slag;

TEST(DescriptorMemory, ResourceDescriptorGroupOffset)
{
    if (slagGraphicsCard()->descriptorBufferOffsetAlignment() > 0)
    {
        auto resourceMemory = std::unique_ptr<ResourceDescriptorMemory>(ResourceDescriptorMemory::newResourceDescriptorMemory(1000));
        GTEST_ASSERT_EQ(resourceMemory->descriptorGroupOffset(0),0);
        GTEST_ASSERT_EQ(resourceMemory->descriptorGroupOffset(1),slagGraphicsCard()->descriptorBufferOffsetAlignment());
        GTEST_ASSERT_EQ(resourceMemory->descriptorGroupOffset(slagGraphicsCard()->descriptorBufferOffsetAlignment()+1),slagGraphicsCard()->descriptorBufferOffsetAlignment()*2);
        GTEST_ASSERT_EQ(resourceMemory->descriptorGroupOffset(slagGraphicsCard()->descriptorBufferOffsetAlignment()),slagGraphicsCard()->descriptorBufferOffsetAlignment());
        GTEST_ASSERT_EQ(resourceMemory->descriptorGroupOffset(slagGraphicsCard()->descriptorBufferOffsetAlignment()*2),slagGraphicsCard()->descriptorBufferOffsetAlignment()*2);
    }
    else
    {
        GTEST_SKIP();
    }
}
TEST(DescriptorMemory, SamplerDescriptorGroupOffset)
{
    if (slagGraphicsCard()->descriptorBufferOffsetAlignment() > 0)
    {
        auto samplerMemory = std::unique_ptr<SamplerDescriptorMemory>(SamplerDescriptorMemory::newSamplerDescriptorMemory(1000));
        GTEST_ASSERT_EQ(samplerMemory->descriptorGroupOffset(0),0);
        GTEST_ASSERT_EQ(samplerMemory->descriptorGroupOffset(1),slagGraphicsCard()->descriptorBufferOffsetAlignment());
        GTEST_ASSERT_EQ(samplerMemory->descriptorGroupOffset(slagGraphicsCard()->descriptorBufferOffsetAlignment()+1),slagGraphicsCard()->descriptorBufferOffsetAlignment()*2);
        GTEST_ASSERT_EQ(samplerMemory->descriptorGroupOffset(slagGraphicsCard()->descriptorBufferOffsetAlignment()),slagGraphicsCard()->descriptorBufferOffsetAlignment());
        GTEST_ASSERT_EQ(samplerMemory->descriptorGroupOffset(slagGraphicsCard()->descriptorBufferOffsetAlignment()*2),slagGraphicsCard()->descriptorBufferOffsetAlignment()*2);
    }
    else
    {
        GTEST_SKIP();
    }
}
TEST(DescriptorMemory, SetSampledTexture)
{
    GTEST_FAIL();
}
TEST(DescriptorMemory, SetStorageTexture)
{
    GTEST_FAIL();
}
TEST(DescriptorMemory, SetUniformTexelBuffer)
{
    GTEST_FAIL();
}
TEST(DescriptorMemory, SetStorageTexelBuffer)
{
    GTEST_FAIL();
}
TEST(DescriptorMemory, SetUniformBuffer)
{
    GTEST_FAIL();
}
TEST(DescriptorMemory, SetStorageBuffer)
{
    GTEST_FAIL();
}
TEST(DescriptorMemory, SetSampler)
{
    GTEST_FAIL();
}