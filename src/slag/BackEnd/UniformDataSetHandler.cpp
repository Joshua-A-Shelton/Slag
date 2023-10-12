#include "UniformDataSetHandler.h"
#ifdef SLAG_VULKAN_BACKEND
#include "Vulkan/VulkanUniformDataSetFunctionProvider.h"
#endif
namespace slag
{
    UniformDataSetFunctionProvider* UNIFORM_DATA_SET_FUNCTION_PROVIDER = nullptr;

    void UniformDataSetHandler::initialize(BackEnd backEnd)
    {
        switch (backEnd)
        {
            case BackEnd::VULKAN:
#ifdef SLAG_VULKAN_BACKEND
            UNIFORM_DATA_SET_FUNCTION_PROVIDER = new slag::vulkan::VulkanUniformDataSetFunctionProvider();
#endif
                break;
            case BackEnd::DX12:
                break;
        }
    }

    void UniformDataSetHandler::cleanup()
    {
        if(UNIFORM_DATA_SET_FUNCTION_PROVIDER!= nullptr)
        {
            delete UNIFORM_DATA_SET_FUNCTION_PROVIDER;
        }
    }

    UniformDataSetFunctionProvider *UniformDataSetHandler::dataProvider()
    {
        return UNIFORM_DATA_SET_FUNCTION_PROVIDER;
    }
} // slag