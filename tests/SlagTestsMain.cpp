#define SDL_MAIN_HANDLED
#include <gtest/gtest.h>
#include <slag/Slag.h>

#include "DX12Environment.h"
#include "VulkanEnvironment.h"
#ifdef SLAG_DX12_BACKEND
#include <intsafe.h>
//TODO: see if there's a way to automate this, this part must be in the final executable, not a library
// For Direct3D 12 Agility SDK
extern "C"
{
    __declspec(dllexport) extern const UINT D3D12SDKVersion = 616;
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}
#endif

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
#ifdef SLAG_DX12_BACKEND
    //::testing::AddGlobalTestEnvironment(new slag::DX12Environment());
#endif
#ifdef SLAG_VULKAN_BACKEND
    ::testing::AddGlobalTestEnvironment(new slag::VulkanEnvironment());
#endif

    auto run = RUN_ALL_TESTS();
    return run;
}
