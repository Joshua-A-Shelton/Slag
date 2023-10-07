#ifndef SLAG_VULKANSHADER_H
#define SLAG_VULKANSHADER_H
#include "../../Shader.h"
#include <vector>
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {

        class VulkanShader: public Shader
        {
        public:
            VulkanShader(const std::vector<char>& vertexCode, const std::vector<char>& fragmentCode);
            ~VulkanShader()override;
        private:
            VkPipelineLayout _pipelineLayout;
            VkPipeline _pipeline;

        };

    } // slag
} // vulkan

#endif //SLAG_VULKANSHADER_H
