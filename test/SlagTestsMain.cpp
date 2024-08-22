#include <gtest/gtest.h>
#include "DX12Environment.h"
namespace slag
{
    namespace tests
    {
        int main(int argc, char **argv)
        {
            ::testing::InitGoogleTest(&argc, argv);
            ::testing::AddGlobalTestEnvironment(new DX12Environment());
            return RUN_ALL_TESTS();
        }
    }
}
