#define SDL_MAIN_HANDLED
#include <gtest/gtest.h>
#include <slag/Slag.h>
#define SDL_MAIN_HANDLED 1
#include <SDL3/SDL.h>

//TODO: see if there's a way to automate this, this part must be in the final executable, not a library
// For Direct3D 12 Agility SDK
#ifdef SLAG_DX12_BACKEND
#include <intsafe.h>
extern "C"
{
    __declspec(dllexport) extern const UINT D3D12SDKVersion = 616;
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}
#endif

void DebugHandler(const std::string& message, slag::DebugLevel debugLevel, int32_t messageID)
{
    std::cout << message << std::endl;
    if (debugLevel != slag::DebugLevel::INFO)
    {
        GTEST_FAIL();
    }
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    bool isBackend = false;
    std::string backendName = "";
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (isBackend)
        {
            backendName = arg;
            isBackend = false;
        }
        else if (arg == "--backend" || arg == "-B")
        {
            isBackend = true;
        }
    }

    slag::InitializationData initData{};
    initData.debugHandler = DebugHandler;
    if (backendName == "vulkan")
    {
        initData.backend = slag::BackendAPI::VULKAN;
    }
    else if (backendName == "dx12")
    {
        initData.backend = slag::BackendAPI::DX12;
    }
    else
    {
        return 1;
    }
    if (slag::Slag::initialize(initData) == slag::SlagInitializationResult::SUCCESS)
    {
        SDL_Init(SDL_INIT_VIDEO);
        auto run = RUN_ALL_TESTS();
        SDL_Quit();
        slag::Slag::cleanup();
        return run;
    }
    return 1;

}