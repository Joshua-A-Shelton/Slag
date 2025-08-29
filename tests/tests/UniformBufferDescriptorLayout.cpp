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

    UniformBufferDescriptorLayout layout3("Layout3", GraphicsType::STRUCT, 1, std::vector<UniformBufferDescriptorLayout>
        {
            UniformBufferDescriptorLayout("Child1_1",GraphicsType::MATRIX_4X4,1,std::vector<UniformBufferDescriptorLayout>(),64,0,0),
            UniformBufferDescriptorLayout("Child1_2",GraphicsType::STRUCT,1,std::vector<UniformBufferDescriptorLayout>
                {
                    UniformBufferDescriptorLayout("GrandChild1_1",GraphicsType::MATRIX_4X4,1,std::vector<UniformBufferDescriptorLayout>(),64,0,64),
                    UniformBufferDescriptorLayout("GrandChild1_2", GraphicsType::VECTOR4,2,std::vector<UniformBufferDescriptorLayout>(),32,64,64*2),
                    UniformBufferDescriptorLayout("GrandChild1_3", GraphicsType::VECTOR4,2,std::vector<UniformBufferDescriptorLayout>(),32,64+32,64*2+32),
                },64*2,64,64),
        }
        ,64*3,0,0);

    UniformBufferDescriptorLayout layout4("Layout4", GraphicsType::STRUCT, 1, std::vector<UniformBufferDescriptorLayout>
   {
       UniformBufferDescriptorLayout("Child1_1",GraphicsType::MATRIX_4X4,1,std::vector<UniformBufferDescriptorLayout>(),64,0,0),
       UniformBufferDescriptorLayout("Child1_2",GraphicsType::STRUCT,1,std::vector<UniformBufferDescriptorLayout>
           {
               UniformBufferDescriptorLayout("GrandChild1_1",GraphicsType::MATRIX_4X4,1,std::vector<UniformBufferDescriptorLayout>(),64,0,64),
               UniformBufferDescriptorLayout("GrandChild1_2", GraphicsType::VECTOR4,2,std::vector<UniformBufferDescriptorLayout>(),32,64,64*2),
               UniformBufferDescriptorLayout("GrandChild1_3", GraphicsType::VECTOR4,2,std::vector<UniformBufferDescriptorLayout>(),32,64+32,64*2+32),
               UniformBufferDescriptorLayout("GrandChild1_4,",GraphicsType::MATRIX_4X4,1,std::vector<UniformBufferDescriptorLayout>(),64,64*2,64*3)
           },64*4,64,64),

   }
    ,64*4,0,0);

    auto combined2 = UniformBufferDescriptorLayout::merge(layout3,layout4);
    GTEST_ASSERT_TRUE(combined2.size()==64*4);
    GTEST_ASSERT_TRUE(combined2.childrenCount() == 2);
    GTEST_ASSERT_TRUE(layoutEqual(combined2.child(0),layout3.child(0)));
    GTEST_ASSERT_TRUE(layoutEqual(combined2.child(1),layout4.child(1)));

    auto combined3 = UniformBufferDescriptorLayout::merge(layout4,layout3);
    GTEST_ASSERT_TRUE(combined3.size()==64*4);
    GTEST_ASSERT_TRUE(combined3.childrenCount() == 2);
    GTEST_ASSERT_TRUE(layoutEqual(combined3.child(0),layout4.child(0)));
    GTEST_ASSERT_TRUE(layoutEqual(combined3.child(1),layout4.child(1)));

}

TEST(UniformBufferDescriptorLayout, MergeFail)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");


    UniformBufferDescriptorLayout layout1("Layout1", GraphicsType::STRUCT, 1, std::vector<UniformBufferDescriptorLayout>
        {
            UniformBufferDescriptorLayout("Child1_1",GraphicsType::MATRIX_4X4,1,std::vector<UniformBufferDescriptorLayout>(),64,0,0),
            UniformBufferDescriptorLayout("Child1_2",GraphicsType::MATRIX_4X4,1,std::vector<UniformBufferDescriptorLayout>(),64,64,64),
            UniformBufferDescriptorLayout("Child1_3", GraphicsType::VECTOR4,2,std::vector<UniformBufferDescriptorLayout>(),32,64*2,64*2),
            UniformBufferDescriptorLayout("Child1_4", GraphicsType::VECTOR4,2,std::vector<UniformBufferDescriptorLayout>(),32,64*2+32,64*2),
        }
        ,64*3,0,0);

    UniformBufferDescriptorLayout layout2("Layout2", GraphicsType::STRUCT, 1, std::vector<UniformBufferDescriptorLayout>
        {
            UniformBufferDescriptorLayout("Child2_1",GraphicsType::MATRIX_4X4,1,std::vector<UniformBufferDescriptorLayout>(),64,0,0),
            UniformBufferDescriptorLayout("Child2_2",GraphicsType::MATRIX_4X4,1,std::vector<UniformBufferDescriptorLayout>(),64,64,64),
            UniformBufferDescriptorLayout("Child2_2",GraphicsType::MATRIX_4X4,1,std::vector<UniformBufferDescriptorLayout>(),64,64*2,64),
        }
        ,64*3,0,0);


    UniformBufferDescriptorLayout layout3("Layout3", GraphicsType::STRUCT, 1, std::vector<UniformBufferDescriptorLayout>
        {
            UniformBufferDescriptorLayout("Child1_1",GraphicsType::MATRIX_4X4,1,std::vector<UniformBufferDescriptorLayout>(),64,0,0),
            UniformBufferDescriptorLayout("Child1_2",GraphicsType::STRUCT,1,std::vector<UniformBufferDescriptorLayout>
                {
                    UniformBufferDescriptorLayout("GrandChild1_1",GraphicsType::MATRIX_4X4,1,std::vector<UniformBufferDescriptorLayout>(),64,0,64),
                    UniformBufferDescriptorLayout("GrandChild1_2", GraphicsType::VECTOR4,2,std::vector<UniformBufferDescriptorLayout>(),32,64,64*2),
                    UniformBufferDescriptorLayout("GrandChild1_3", GraphicsType::VECTOR4,2,std::vector<UniformBufferDescriptorLayout>(),32,64+32,64*2+32),
                },64*2,64,64),
        }
        ,64*3,0,0);

        UniformBufferDescriptorLayout layout4("Layout4", GraphicsType::STRUCT, 1, std::vector<UniformBufferDescriptorLayout>
       {
           UniformBufferDescriptorLayout("Child1_1",GraphicsType::MATRIX_4X4,1,std::vector<UniformBufferDescriptorLayout>(),64,0,0),
           UniformBufferDescriptorLayout("Child1_2",GraphicsType::STRUCT,1,std::vector<UniformBufferDescriptorLayout>
               {
                   UniformBufferDescriptorLayout("GrandChild1_1",GraphicsType::MATRIX_4X4,1,std::vector<UniformBufferDescriptorLayout>(),64,0,64),
                   UniformBufferDescriptorLayout("GrandChild1_2", GraphicsType::VECTOR4,2,std::vector<UniformBufferDescriptorLayout>(),32,64,64*2),
                   UniformBufferDescriptorLayout("GrandChild1_3", GraphicsType::VECTOR2,4,std::vector<UniformBufferDescriptorLayout>(),32,64+32,64*2+32),
                   UniformBufferDescriptorLayout("GrandChild1_4,",GraphicsType::MATRIX_4X4,1,std::vector<UniformBufferDescriptorLayout>(),64,64*2,64*3)
               },64*4,64,64),

       }
        ,64*4,0,0);

    EXPECT_THROW(UniformBufferDescriptorLayout::merge(layout1,layout2),std::runtime_error);
    EXPECT_THROW(UniformBufferDescriptorLayout::merge(layout3,layout4),std::runtime_error);
}