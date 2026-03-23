#ifndef SLAG_VULKANSHADERMODULE_H
#define SLAG_VULKANSHADERMODULE_H
#include <slag/Slag.h>
#include <vulkan/vulkan.h>

namespace slag
{
    namespace vulkan
    {
        class VulkanGraphicsCard;
        class VulkanShaderModule: public ShaderModule
        {
        public:
            VulkanShaderModule(VulkanGraphicsCard* graphicsCard,void* data, uint32_t dataLength);
            ~VulkanShaderModule()override;

            [[nodiscard]] ShaderLanguage shaderLanguage() const override;
            [[nodiscard]] const ShaderMetaData& metaData()override;

        private:
            ShaderLanguage _shaderLanguage = ShaderLanguage::SPIRV;
            ShaderMetaData _metaData{};
            VkShaderModule _module = nullptr;
            VulkanGraphicsCard* _graphicsCard = nullptr;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANSHADERMODULE_H
