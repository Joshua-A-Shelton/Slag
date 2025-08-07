#include <gtest/gtest.h>
#include <slag/Slag.h>
using namespace slag;

bool layoutEqual(const UniformBufferDescriptorLayout& a, const UniformBufferDescriptorLayout& b)
{
    return a.name() == b.name() && a.type() == b.type() && a.size() == b.size() && a.offset() == b.offset() && a.arrayDepth() == b.arrayDepth() && a.childrenCount() == b.childrenCount();
}
TEST(UniformBufferDescriptorLayout, MergeCompatible)
{
    UniformBufferDescriptorLayout layout1("Layout1", GraphicsType::STRUCT, 1, std::vector<UniformBufferDescriptorLayout>
        {
            UniformBufferDescriptorLayout("Child1_1",GraphicsType::MATRIX_4X4,1,std::vector<UniformBufferDescriptorLayout>(),64,64,64),
            UniformBufferDescriptorLayout("Child1_2", GraphicsType::VECTOR4,1,std::vector<UniformBufferDescriptorLayout>(),16,64*2,64*2),
            UniformBufferDescriptorLayout("Child1_3", GraphicsType::VECTOR4,2,std::vector<UniformBufferDescriptorLayout>(),32,64*2+32,64*2),
        }
        ,64*3,0,0);

    UniformBufferDescriptorLayout layout2("Layout2", GraphicsType::STRUCT, 1, std::vector<UniformBufferDescriptorLayout>
        {
            UniformBufferDescriptorLayout("Child2_1",GraphicsType::MATRIX_4X4,1,std::vector<UniformBufferDescriptorLayout>(),64,0,0),
            UniformBufferDescriptorLayout("Child2_2",GraphicsType::MATRIX_4X4,1,std::vector<UniformBufferDescriptorLayout>(),64,64,64),
            UniformBufferDescriptorLayout("Child2_3", GraphicsType::VECTOR4,2,std::vector<UniformBufferDescriptorLayout>(),32,64*2,64*2),
            UniformBufferDescriptorLayout("Child2_4", GraphicsType::VECTOR4,2,std::vector<UniformBufferDescriptorLayout>(),32,64*2+32,64*2),
        }
        ,64*3,0,0);

    auto combined = UniformBufferDescriptorLayout::merge(layout1,layout2);

    GTEST_ASSERT_TRUE(combined.size()==64*3);
    GTEST_ASSERT_TRUE(combined.childrenCount() == 4);
    GTEST_ASSERT_TRUE(layoutEqual(combined.child(0),layout2.child(0)));
    GTEST_ASSERT_TRUE(layoutEqual(combined.child(1),layout1.child(0)));
    GTEST_ASSERT_TRUE(layoutEqual(combined.child(2),layout2.child(2)));
    GTEST_ASSERT_TRUE(layoutEqual(combined.child(3),layout1.child(2)));

}

TEST(UniformBufferDescriptorLayout, MergeFail)
{
    GTEST_FAIL();
}