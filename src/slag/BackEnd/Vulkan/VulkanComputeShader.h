#ifndef SLAG_VULKANCOMPUTESHADER_H
#define SLAG_VULKANCOMPUTESHADER_H
#include <vulkan/vulkan.h>
#include "../../ComputeShader.h"
#include "../Resource.h"
#include "VulkanUniformSet.h"
#include "VulkanPushConstantRange.h"

namespace slag
{
    namespace vulkan
    {

        class VulkanComputeShader: public ComputeShader, Resource
        {
        public:
            VulkanComputeShader(const std::vector<char>& code);
            ~VulkanComputeShader()override;
            UniformSet* getUniformSet(size_t index)override;
            size_t uniformSetCount()override;
            PushConstantRange* getPushConstantRange(size_t index)override;
            size_t pushConstantRangeCount()override;
            void* GPUID()override;
            VkPipelineLayout layout();
            VkPipeline pipeline();
        private:
            VkPipelineLayout _pipelineLayout = nullptr;
            VkPipeline _pipeline = nullptr;
            std::vector<VulkanUniformSet> _uniformSets;
            std::vector<VulkanPushConstantRange> _pushConstantRanges;
        };

    } // vulkan
} // slag

#endif //CRUCIBLEEDITOR_VULKANCOMPUTESHADER_H
