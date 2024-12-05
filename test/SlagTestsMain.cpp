#include <gtest/gtest.h>
#include "DX12Environment.h"
#include "VulkanEnvironment.h"
#include <slag/SlagLib.h>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <intsafe.h>
#ifdef _WIN32
//TODO: see if there's a way to automate this, this part must be in the final executable, not a library
// For Direct3D 12 Agility SDK
extern "C"
{
__declspec(dllexport) extern const UINT D3D12SDKVersion = 614;
__declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}
#endif
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
    SDL_Init(SDL_INIT_EVERYTHING);
    auto run = RUN_ALL_TESTS();
    return run;
}
