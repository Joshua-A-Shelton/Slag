#ifndef SLAG_VULKANSHADER_H
#define SLAG_VULKANSHADER_H
#include "../../Shader.h"
#include "../Resource.h"
#include <vector>
#include <vulkan/vulkan.h>
#include "VulkanVertexDescription.h"
namespace slag
{
    namespace vulkan
    {

        class VulkanShader: public Shader, Resource
        {
        public:
            VulkanShader(const std::vector<char>& vertexCode, const std::vector<char>& fragmentCode, VertexDescription& vertexDescription);
            ~VulkanShader()override;
        private:
            VkPipelineLayout _pipelineLayout;
            VkPipeline _pipeline;
            VulkanVertexDescription _vertexDescription;

        };

    } // slag
} // vulkan

#endif //SLAG_VULKANSHADER_H
