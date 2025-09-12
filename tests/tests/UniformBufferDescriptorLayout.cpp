#include <gtest/gtest.h>
#include <slag/Slag.h>
using namespace slag;

bool layoutEqual(const BufferLayout& a, const BufferLayout& b)
{
    return a.name() == b.name() && a.type() == b.type() && a.size() == b.size() && a.offset() == b.offset() && a.arrayDepth() == b.arrayDepth() && a.childrenCount() == b.childrenCount();
}
TEST(UniformBufferDescriptorLayout, MergeCompatible)
{
    BufferLayout layout1("Layout1", GraphicsType::STRUCT, 1, std::vector<BufferLayout>
        {
            BufferLayout("Child1_1",GraphicsType::MATRIX_4X4,1,std::vector<BufferLayout>(),64,64,64),
            BufferLayout("Child1_2", GraphicsType::VECTOR4,1,std::vector<BufferLayout>(),16,64*2,64*2),
            BufferLayout("Child1_3", GraphicsType::VECTOR4,2,std::vector<BufferLayout>(),32,64*2+32,64*2),
        }
        ,64*3,0,0);

    BufferLayout layout2("Layout2", GraphicsType::STRUCT, 1, std::vector<BufferLayout>
        {
            BufferLayout("Child2_1",GraphicsType::MATRIX_4X4,1,std::vector<BufferLayout>(),64,0,0),
            BufferLayout("Child2_2",GraphicsType::MATRIX_4X4,1,std::vector<BufferLayout>(),64,64,64),
            BufferLayout("Child2_3", GraphicsType::VECTOR4,2,std::vector<BufferLayout>(),32,64*2,64*2),
            BufferLayout("Child2_4", GraphicsType::VECTOR4,2,std::vector<BufferLayout>(),32,64*2+32,64*2),
        }
        ,64*3,0,0);

    auto combined = BufferLayout::merge(layout1,layout2);

    GTEST_ASSERT_TRUE(combined.size()==64*3);
    GTEST_ASSERT_TRUE(combined.childrenCount() == 4);
    GTEST_ASSERT_TRUE(layoutEqual(combined.child(0),layout2.child(0)));
    GTEST_ASSERT_TRUE(layoutEqual(combined.child(1),layout1.child(0)));
    GTEST_ASSERT_TRUE(layoutEqual(combined.child(2),layout2.child(2)));
    GTEST_ASSERT_TRUE(layoutEqual(combined.child(3),layout1.child(2)));

    BufferLayout layout3("Layout3", GraphicsType::STRUCT, 1, std::vector<BufferLayout>
        {
            BufferLayout("Child1_1",GraphicsType::MATRIX_4X4,1,std::vector<BufferLayout>(),64,0,0),
            BufferLayout("Child1_2",GraphicsType::STRUCT,1,std::vector<BufferLayout>
                {
                    BufferLayout("GrandChild1_1",GraphicsType::MATRIX_4X4,1,std::vector<BufferLayout>(),64,0,64),
                    BufferLayout("GrandChild1_2", GraphicsType::VECTOR4,2,std::vector<BufferLayout>(),32,64,64*2),
                    BufferLayout("GrandChild1_3", GraphicsType::VECTOR4,2,std::vector<BufferLayout>(),32,64+32,64*2+32),
                },64*2,64,64),
        }
        ,64*3,0,0);

    BufferLayout layout4("Layout4", GraphicsType::STRUCT, 1, std::vector<BufferLayout>
   {
       BufferLayout("Child1_1",GraphicsType::MATRIX_4X4,1,std::vector<BufferLayout>(),64,0,0),
       BufferLayout("Child1_2",GraphicsType::STRUCT,1,std::vector<BufferLayout>
           {
               BufferLayout("GrandChild1_1",GraphicsType::MATRIX_4X4,1,std::vector<BufferLayout>(),64,0,64),
               BufferLayout("GrandChild1_2", GraphicsType::VECTOR4,2,std::vector<BufferLayout>(),32,64,64*2),
               BufferLayout("GrandChild1_3", GraphicsType::VECTOR4,2,std::vector<BufferLayout>(),32,64+32,64*2+32),
               BufferLayout("GrandChild1_4,",GraphicsType::MATRIX_4X4,1,std::vector<BufferLayout>(),64,64*2,64*3)
           },64*4,64,64),

   }
    ,64*4,0,0);

    auto combined2 = BufferLayout::merge(layout3,layout4);
    GTEST_ASSERT_TRUE(combined2.size()==64*4);
    GTEST_ASSERT_TRUE(combined2.childrenCount() == 2);
    GTEST_ASSERT_TRUE(layoutEqual(combined2.child(0),layout3.child(0)));
    GTEST_ASSERT_TRUE(layoutEqual(combined2.child(1),layout4.child(1)));

    auto combined3 = BufferLayout::merge(layout4,layout3);
    GTEST_ASSERT_TRUE(combined3.size()==64*4);
    GTEST_ASSERT_TRUE(combined3.childrenCount() == 2);
    GTEST_ASSERT_TRUE(layoutEqual(combined3.child(0),layout4.child(0)));
    GTEST_ASSERT_TRUE(layoutEqual(combined3.child(1),layout4.child(1)));

}

TEST(UniformBufferDescriptorLayout, MergeFail)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");


    BufferLayout layout1("Layout1", GraphicsType::STRUCT, 1, std::vector<BufferLayout>
        {
            BufferLayout("Child1_1",GraphicsType::MATRIX_4X4,1,std::vector<BufferLayout>(),64,0,0),
            BufferLayout("Child1_2",GraphicsType::MATRIX_4X4,1,std::vector<BufferLayout>(),64,64,64),
            BufferLayout("Child1_3", GraphicsType::VECTOR4,2,std::vector<BufferLayout>(),32,64*2,64*2),
            BufferLayout("Child1_4", GraphicsType::VECTOR4,2,std::vector<BufferLayout>(),32,64*2+32,64*2),
        }
        ,64*3,0,0);

    BufferLayout layout2("Layout2", GraphicsType::STRUCT, 1, std::vector<BufferLayout>
        {
            BufferLayout("Child2_1",GraphicsType::MATRIX_4X4,1,std::vector<BufferLayout>(),64,0,0),
            BufferLayout("Child2_2",GraphicsType::MATRIX_4X4,1,std::vector<BufferLayout>(),64,64,64),
            BufferLayout("Child2_2",GraphicsType::MATRIX_4X4,1,std::vector<BufferLayout>(),64,64*2,64),
        }
        ,64*3,0,0);


    BufferLayout layout3("Layout3", GraphicsType::STRUCT, 1, std::vector<BufferLayout>
        {
            BufferLayout("Child1_1",GraphicsType::MATRIX_4X4,1,std::vector<BufferLayout>(),64,0,0),
            BufferLayout("Child1_2",GraphicsType::STRUCT,1,std::vector<BufferLayout>
                {
                    BufferLayout("GrandChild1_1",GraphicsType::MATRIX_4X4,1,std::vector<BufferLayout>(),64,0,64),
                    BufferLayout("GrandChild1_2", GraphicsType::VECTOR4,2,std::vector<BufferLayout>(),32,64,64*2),
                    BufferLayout("GrandChild1_3", GraphicsType::VECTOR4,2,std::vector<BufferLayout>(),32,64+32,64*2+32),
                },64*2,64,64),
        }
        ,64*3,0,0);

        BufferLayout layout4("Layout4", GraphicsType::STRUCT, 1, std::vector<BufferLayout>
       {
           BufferLayout("Child1_1",GraphicsType::MATRIX_4X4,1,std::vector<BufferLayout>(),64,0,0),
           BufferLayout("Child1_2",GraphicsType::STRUCT,1,std::vector<BufferLayout>
               {
                   BufferLayout("GrandChild1_1",GraphicsType::MATRIX_4X4,1,std::vector<BufferLayout>(),64,0,64),
                   BufferLayout("GrandChild1_2", GraphicsType::VECTOR4,2,std::vector<BufferLayout>(),32,64,64*2),
                   BufferLayout("GrandChild1_3", GraphicsType::VECTOR2,4,std::vector<BufferLayout>(),32,64+32,64*2+32),
                   BufferLayout("GrandChild1_4,",GraphicsType::MATRIX_4X4,1,std::vector<BufferLayout>(),64,64*2,64*3)
               },64*4,64,64),

       }
        ,64*4,0,0);

    EXPECT_THROW(BufferLayout::merge(layout1,layout2),std::runtime_error);
    EXPECT_THROW(BufferLayout::merge(layout3,layout4),std::runtime_error);
}