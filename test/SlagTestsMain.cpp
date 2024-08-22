#include <gtest/gtest.h>
#include "DX12Environment.h"
#include "VulkanEnvironment.h"
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    GTEST_FLAG_SET(death_test_style, "fast");
    ::testing::AddGlobalTestEnvironment(new slag::tests::DX12Environment());
    ::testing::AddGlobalTestEnvironment(new slag::tests::VulkanEnvironment());
    return RUN_ALL_TESTS();
}
