#ifndef SLAG_VULKANSHADER_H
#define SLAG_VULKANSHADER_H
#include "../../Shader.h"
#include "../Resource.h"
#include <vector>
#include <vulkan/vulkan.h>
#include "VulkanUniformSet.h"
#include "../../FramebufferDescription.h"

namespace slag
{
    namespace vulkan
    {

        class VulkanShader: public Shader, Resource
        {
        public:
            VulkanShader(const std::vector<char>& vertexCode, const std::vector<char>& fragmentCode, FramebufferDescription& framebufferDescription);
            ~VulkanShader()override;
            VulkanShader(const VulkanShader&)=delete;
            VulkanShader& operator=(const VulkanShader&)=delete;
            VulkanShader(VulkanShader&& from);
            VulkanShader& operator=(VulkanShader&& from);
            void* GPUID()override;
            UniformSet* getUniformSet(size_t index)override;
            VkPipeline pipeline();
            VkPipelineLayout layout();
        private:
            VkPipelineLayout _pipelineLayout = nullptr;
            VkPipeline _pipeline = nullptr;
            std::vector<VulkanUniformSet> _uniformSets;
            std::vector<VkPushConstantRange> _pushConstantRanges;
            void generateReflectionData(const std::vector<char>& vertexCode, const std::vector<char>& fragmentCode, std::vector<VkVertexInputAttributeDescription>& attributes,VkVertexInputBindingDescription& binding, std::vector<VulkanUniformSet>& overwrites);
            void move(VulkanShader&& from);
        };

    } // slag
} // vulkan

#endif //SLAG_VULKANSHADER_H
