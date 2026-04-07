#include "VulkanShaderModule.h"
#include "VulkanGraphicsCard.h"
#include "slag/exceptions/InvalidShaderCodeError.h"
#include "slag/shader-reflection/spirv/SPIRVShaderReflector.h"

namespace slag
{
    namespace vulkan
    {
        VulkanShaderModule::VulkanShaderModule(VulkanGraphicsCard* graphicsCard, void* data, uint32_t dataLength)
        {
            _graphicsCard = graphicsCard;
            VkShaderModuleCreateInfo shaderModuleCreateInfo = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
            shaderModuleCreateInfo.codeSize = dataLength;
            shaderModuleCreateInfo.pCode = static_cast<uint32_t*>(data);
            if (vkCreateShaderModule(_graphicsCard->device(),&shaderModuleCreateInfo,nullptr,&_module)!= VK_SUCCESS)
            {
                throw InvalidShaderCodeError("Provided code is not valid SPIRV");
            }
            try
            {
                _metaData = SPIRVShaderReflector::GetMetaData(data,dataLength);
            }
            catch (...)
            {
                vkDestroyShaderModule(_graphicsCard->device(),_module,nullptr);
                throw;
            }

        }

        VulkanShaderModule::~VulkanShaderModule()
        {
            if (_module)
            {
                vkDestroyShaderModule(_graphicsCard->device(),_module,nullptr);
            }
        }

        ShaderLanguage VulkanShaderModule::shaderLanguage() const
        {
            return _shaderLanguage;
        }

        const ShaderMetaData& VulkanShaderModule::metaData()
        {
            return _metaData;
        }

        GraphicsCard* VulkanShaderModule::graphicsCard() const
        {
            return _graphicsCard;
        }

        VkShaderModule VulkanShaderModule::nativeModule() const
        {
            return _module;
        }
    } // vulkan
} // slag
