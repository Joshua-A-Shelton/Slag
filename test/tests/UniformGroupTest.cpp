#include "gtest/gtest.h"
#include "slag/SlagLib.h"
using namespace slag;
TEST(UniformGroup, Shape)
{
    std::vector<Descriptor> descriptors;
    descriptors.push_back(Descriptor("Albedo",Descriptor::SAMPLER_AND_TEXTURE,1,0,ShaderStageFlags::FRAGMENT));
    descriptors.push_back(Descriptor("MVP",Descriptor::UNIFORM_BUFFER,1,1,ShaderStageFlags::VERTEX | ShaderStageFlags::FRAGMENT));
    descriptors.push_back(Descriptor("Shadow",Descriptor::SAMPLED_TEXTURE,1,2,ShaderStageFlags::FRAGMENT));
    descriptors.push_back(Descriptor("Wind",Descriptor::UNIFORM_BUFFER,1,3,ShaderStageFlags::VERTEX));
    auto group = DescriptorGroup::newDescriptorGroup(descriptors.data(),descriptors.size());
    std::vector<Descriptor> descriptors2;
    descriptors2.push_back(Descriptor("Wind",Descriptor::UNIFORM_BUFFER,1,3,ShaderStageFlags::VERTEX));
    descriptors2.push_back(Descriptor("Shadow",Descriptor::SAMPLED_TEXTURE,1,2,ShaderStageFlags::FRAGMENT));
    descriptors2.push_back(Descriptor("MVP",Descriptor::UNIFORM_BUFFER,1,1,ShaderStageFlags::VERTEX | ShaderStageFlags::FRAGMENT));
    descriptors2.push_back(Descriptor("Albedo",Descriptor::SAMPLER_AND_TEXTURE,1,0,ShaderStageFlags::FRAGMENT));
    auto group2 = DescriptorGroup::newDescriptorGroup(descriptors2.data(),descriptors2.size());

    descriptors.push_back(Descriptor("DifferentThing",Descriptor::INPUT_ATTACHMENT,1,4,slag::ShaderStageFlags::FRAGMENT));
    auto group3 = DescriptorGroup::newDescriptorGroup(descriptors.data(),descriptors.size());
    descriptors2.push_back(Descriptor("DifferentThing",Descriptor::INPUT_ATTACHMENT,1,4,slag::ShaderStageFlags::FRAGMENT | ShaderStageFlags::VERTEX));
    auto group4 = DescriptorGroup::newDescriptorGroup(descriptors2.data(),descriptors2.size());
    GTEST_ASSERT_TRUE(group->shape() == group2->shape());
    GTEST_ASSERT_TRUE(group->shape() != group3->shape());
    GTEST_ASSERT_TRUE(group2->shape() != group4->shape());
    GTEST_ASSERT_TRUE(group3->shape() != group4->shape());
    delete group;
    delete group2;
}