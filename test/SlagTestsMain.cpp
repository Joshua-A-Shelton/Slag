#include <gtest/gtest.h>
#include "DX12Environment.h"
#include "VulkanEnvironment.h"
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    bool useVulkan = true;
#if _WIN32
    useVulkan = false;
#endif
    for(int i=0; i< argc; i++)
    {
        char* arg = argv[i];
        std::string argument(arg);
        if(argument == "vulkan")
        {
            useVulkan = true;
            break;
        }
        else if(argument == "dx12")
        {
            useVulkan = false;
        }
    }
    if(useVulkan)
    {
        ::testing::AddGlobalTestEnvironment(new slag::tests::VulkanEnvironment());
    }
    else
    {
        ::testing::AddGlobalTestEnvironment(new slag::tests::DX12Environment());
    }

    return RUN_ALL_TESTS();
}
