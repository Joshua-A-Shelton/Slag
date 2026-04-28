#ifndef SLAG_VULKANSHADERPIPELINE_H
#define SLAG_VULKANSHADERPIPELINE_H
#include <slag/Slag.h>
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanGraphicsCard;

        class VulkanShaderPipeline: public ShaderPipeline
        {
        public:
            VulkanShaderPipeline(
                VulkanGraphicsCard* graphicsCard,
                const VertexDescription& vertexDescription,
                ShaderModule* vertexShader,
                ShaderModule* fragmentShader,
                PipelineInputMapping* inputBindings,
                const PipelineState& pipelineState,
                const FramebufferDescription& framebufferDescription);
            ~VulkanShaderPipeline()override;
            VulkanShaderPipeline(const VulkanShaderPipeline&)=delete;
            VulkanShaderPipeline& operator=(const VulkanShaderPipeline&)=delete;
            VulkanShaderPipeline(VulkanShaderPipeline&& from) noexcept;
            VulkanShaderPipeline& operator=(VulkanShaderPipeline&& from) noexcept;

            ShaderPipelineType type()override;
            GraphicsCard* graphicsCard()override;
            const PipelineInputMapping& bindings()override;
            VkPipeline vulkanHandle() const;
        private:
            static void initNativeRasterizationInfo(const RasterizationState& slagRasterizationState, VkPipelineRasterizationStateCreateInfo* outRasterizationStateInfo);
            static void initMultisampleInfo(const MultiSampleState& multiSampleState, VkPipelineMultisampleStateCreateInfo* outMultisampleStateInfo);
            static std::vector<VkPipelineColorBlendAttachmentState> initColorAttachmentInfo(const BlendState& slagBlendState, const FramebufferDescription& frameBufferDescription, VkPipelineColorBlendStateCreateInfo* outPipelineColorBlendStateInfo);
            static void initDepthAttachmentInfo(const DepthStencilState& slagDepthStencilState, VkPipelineDepthStencilStateCreateInfo* outDepthStencilStateInfo);
            void move(VulkanShaderPipeline& from);
            PipelineInputMapping _bindings = PipelineInputMapping(std::vector<PipelineInput>());
            VulkanGraphicsCard* _graphicsCard = nullptr;
            VkPipeline _pipeline = nullptr;
            ShaderPipelineType _type = ShaderPipelineType::GRAPHICS;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANSHADERPIPELINE_H