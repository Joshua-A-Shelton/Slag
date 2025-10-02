#define VMA_IMPLEMENTATION
#include "VulkanBackend.h"
#include "VkBootstrap.h"
#include "core/VulkanBuffer.h"
#include "core/VulkanBufferView.h"
#include "core/VulkanCommandBuffer.h"
#include "core/VulkanDescriptorPool.h"
#include "core/VulkanGraphicsCard.h"
#include "core/VulkanSampler.h"
#include "core/VulkanSemaphore.h"
#include "core/VulkanShaderPipeline.h"
#include "core/VulkanSwapChain.h"
#include "core/VulkanTexture.h"
#include "slag/core/PixelFormatProperties.h"
#include "slag/core/SwapChain.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        std::vector<VulkanizedFormat> VULKAN_PIXEL_FORMATS
        {
#define DEFINITION(SlagName, DxName, VulkanName, VkImageAspectFlags, VkComponentSwizzle_r, VkComponentSwizzle_g, VkComponentSwizzle_b, VkComponentSwizzle_a, totalBits,colorBits,depthBits,stencilBits, aspects) VulkanizedFormat(VulkanName,VkComponentSwizzle_r, VkComponentSwizzle_g, VkComponentSwizzle_b, VkComponentSwizzle_a),
            SLAG_TEXTURE_FORMAT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
        };

        std::vector<VkImageAspectFlags> VULKAN_IMAGE_ASPECTS
        {
            VK_IMAGE_ASPECT_NONE, //0
            VK_IMAGE_ASPECT_COLOR_BIT, //1
            VK_IMAGE_ASPECT_DEPTH_BIT, //2
            VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT, //3
            VK_IMAGE_ASPECT_STENCIL_BIT, //4
            VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_COLOR_BIT, //5
            VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT, //6
            VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT //7
        };

        VulkanizedFormat VulkanBackend::vulkanizedFormat(Pixels::Format format)
        {
            return VULKAN_PIXEL_FORMATS[static_cast<uint32_t>(format)];
        }

        VkImageUsageFlags VulkanBackend::vulkanizedUsage(Texture::UsageFlags flags)
        {
            SLAG_ASSERT((!(bool)(flags & Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT & Texture::UsageFlags::RENDER_TARGET_ATTACHMENT)) && (!(bool)(flags & Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT & Texture::UsageFlags::SAMPLED_IMAGE)) && "Texture cannot be used as both depth and color simultaneously");
            VkImageUsageFlags result = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            if (static_cast<bool>(flags & Texture::UsageFlags::SAMPLED_IMAGE))
            {
                result |= VK_IMAGE_USAGE_SAMPLED_BIT;
            }
            if (static_cast<bool>(flags & Texture::UsageFlags::STORAGE))
            {
                result |= VK_IMAGE_USAGE_STORAGE_BIT;
            }
            if (static_cast<bool>(flags & Texture::UsageFlags::RENDER_TARGET_ATTACHMENT))
            {
                result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            }
            if (static_cast<bool>(flags & Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT))
            {
                result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            }
            return result;
        }

        VkImageType VulkanBackend::vulkanizedImageType(Texture::Type type)
        {
            switch (type)
            {
                case Texture::Type::TEXTURE_1D:
                    return VK_IMAGE_TYPE_1D;
                case Texture::Type::TEXTURE_2D:
                    return VK_IMAGE_TYPE_2D;
                case Texture::Type::TEXTURE_3D:
                    return VK_IMAGE_TYPE_3D;
                case Texture::Type::TEXTURE_CUBE:
                    return VK_IMAGE_TYPE_2D;
                default:
                    return VK_IMAGE_TYPE_2D;
            }
            return VK_IMAGE_TYPE_2D;
        }

        VkImageViewType VulkanBackend::vulkanizedImageViewType(Texture::Type type, uint32_t layers)
        {
            switch (type)
            {
            case Texture::Type::TEXTURE_1D:
                if (layers > 1)
                {
                    return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
                }
                return VK_IMAGE_VIEW_TYPE_1D;
            case Texture::Type::TEXTURE_2D:
                if (layers > 1)
                {
                    return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                }
                return VK_IMAGE_VIEW_TYPE_2D;
            case Texture::Type::TEXTURE_3D:
                return VK_IMAGE_VIEW_TYPE_3D;
            case Texture::Type::TEXTURE_CUBE:
                SLAG_ASSERT(layers%6==0);
                if (layers > 6)
                {
                    return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
                }
                return VK_IMAGE_VIEW_TYPE_CUBE;
            default:
                return VK_IMAGE_VIEW_TYPE_2D;
            }
            return VK_IMAGE_VIEW_TYPE_2D;
        }

        VkImageAspectFlags VulkanBackend::vulkanizedAspectFlags(Pixels::AspectFlags aspectFlags)
        {
            auto index = static_cast<uint8_t>(aspectFlags);
            return VULKAN_IMAGE_ASPECTS[static_cast<uint8_t>(aspectFlags)];
        }

        VkBufferUsageFlags VulkanBackend::vulkanizedBufferUsage(Buffer::UsageFlags usageFlags)
        {
            //all buffers should support copy operations
            VkBufferUsageFlags flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            if ((bool)(usageFlags & Buffer::UsageFlags::VERTEX_BUFFER))
            {
                flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            }
            if ((bool)(usageFlags & Buffer::UsageFlags::INDEX_BUFFER))
            {
                flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            }
            if ((bool)(usageFlags & Buffer::UsageFlags::UNIFORM_BUFFER))
            {
                flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            }
            if ((bool)(usageFlags & Buffer::UsageFlags::STORAGE_BUFFER))
            {
                flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }
            if ((bool)(usageFlags & Buffer::UsageFlags::UNIFORM_TEXEL_BUFFER))
            {
                flags |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
            }
            if ((bool)(usageFlags & Buffer::UsageFlags::STORAGE_TEXEL_BUFFER))
            {
                flags |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
            }
            if ((bool)(usageFlags & Buffer::UsageFlags::INDIRECT_BUFFER))
            {
                flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            }
            return flags;
        }

        VkAccessFlagBits2 VulkanBackend::vulkanizedBarrierAccessMask(BarrierAccessFlags accessFlags)
        {
            VkAccessFlagBits2 bits = 0;
#define DEFINITION(slagName, slagValue, vulkanName, directXName) if((bool)(accessFlags & BarrierAccessFlags::slagName)){ bits |= vulkanName;}
            SLAG_MEMORY_BARRIER_ACCESS_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            return bits;
        }

        VkPipelineStageFlags2 VulkanBackend::vulkanizedStageMask(PipelineStageFlags stageFlags)
        {
            VkPipelineStageFlags2 bits = 0;
#define DEFINITION(slagName, slagValue, vulkanName, directXName) if((bool)(stageFlags & PipelineStageFlags::slagName)){ bits |= vulkanName;}
            SLAG_MEMORY_PIPELINE_STAGE_DEFINITIONS(DEFINITION)
    #undef DEFINITION
            return bits;
        }

        VkIndexType VulkanBackend::vulkanizedIndexType(Buffer::IndexSize indexSize)
        {
            switch (indexSize)
            {
            case Buffer::IndexSize::UINT16:
                return VK_INDEX_TYPE_UINT16;
            case Buffer::IndexSize::UINT32:
                return VK_INDEX_TYPE_UINT32;
            }
            return VK_INDEX_TYPE_UINT16;
        }

        std::vector<VkPolygonMode> VULKAN_POLYGON_MODES =
        {
            VK_POLYGON_MODE_FILL,
            VK_POLYGON_MODE_LINE,
            VK_POLYGON_MODE_POINT
        };

        VkPolygonMode VulkanBackend::vulkanizedPolygonMode(RasterizationState::DrawMode drawMode)
        {
            return VULKAN_POLYGON_MODES[static_cast<uint8_t>(drawMode)];
        }

        std::vector<VkCullModeFlags> VULKAN_CULL_FLAGS
        {
            VK_CULL_MODE_NONE,
            VK_CULL_MODE_FRONT_BIT,
            VK_CULL_MODE_BACK_BIT,
        };
        VkCullModeFlags VulkanBackend::vulkanizedCullMode(RasterizationState::CullOptions cullOptions)
        {
            return VULKAN_CULL_FLAGS[static_cast<uint8_t>(cullOptions)];
        }

        std::vector<VkFrontFace> VULKAN_FRONT_FACES
        {
            VK_FRONT_FACE_CLOCKWISE,
            VK_FRONT_FACE_COUNTER_CLOCKWISE
        };
        VkFrontFace VulkanBackend::vulkanizedFrontFace(RasterizationState::FrontFacing frontFace)
        {
            return VULKAN_FRONT_FACES[static_cast<uint8_t>(frontFace)];
        }

        std::vector<VkBlendFactor> VULKAN_BLEND_FACTORS
        {
#define DEFINITION(SlagName, VulkanName, DXName) VulkanName,
            BLEND_FACTOR_DEFINTITIONS(DEFINITION)
#undef DEFINITION
        };
        VkBlendFactor VulkanBackend::vulkanizedBlendFactor(Operations::BlendFactor blendFactor)
        {
            return VULKAN_BLEND_FACTORS[static_cast<uint8_t>(blendFactor)];
        }

        std::vector<VkBlendOp> VULKAN_BLEND_OPS
        {
#define DEFINITION(SlagName, VulkanName, DXName) VulkanName,
            BLEND_OP_DEFINTITIONS(DEFINITION)
#undef DEFINITION
        };
        VkBlendOp VulkanBackend::vulkanizedBlendOp(Operations::BlendOperation blendOperation)
        {
            return VULKAN_BLEND_OPS[static_cast<uint8_t>(blendOperation)];
        }

        VkColorComponentFlags VulkanBackend::vulkanizedColorComponentFlags(Color::ComponentFlags colorComponentFlags)
        {
            VkColorComponentFlags flags = 0;
            if (static_cast<bool>(colorComponentFlags & Color::ComponentFlags::RED_COMPONENT))
            {
                flags |= VK_COLOR_COMPONENT_R_BIT;
            }
            if (static_cast<bool>(colorComponentFlags & Color::ComponentFlags::GREEN_COMPONENT))
            {
                flags |= VK_COLOR_COMPONENT_G_BIT;
            }
            if (static_cast<bool>(colorComponentFlags & Color::ComponentFlags::BLUE_COMPONENT))
            {
                flags |= VK_COLOR_COMPONENT_B_BIT;
            }
            if (static_cast<bool>(colorComponentFlags & Color::ComponentFlags::ALPHA_COMPONENT))
            {
                flags |= VK_COLOR_COMPONENT_A_BIT;
            }
            return flags;
        }

        std::vector<VkLogicOp> VULKAN_LOGIC_OPS
        {
#define DEFINITION(SlagName,VulkanName,DX12Name) VulkanName,
            FRAMEBUFFER_LOGICAL_OP_DEFINITIONS(DEFINITION)
#undef DEFINITION
        };
        VkLogicOp VulkanBackend::vulkanizedLogicOp(Operations::LogicalOperation operation)
        {
            return VULKAN_LOGIC_OPS[static_cast<uint8_t>(operation)];
        }

        std::vector<VkCompareOp> VULKAN_COMPARE_OPS
        {
#define DEFINITION(slagName, vulkanName, dx12Name) vulkanName,
            COMPARISON_FUNCTION(DEFINITION)
#undef DEFINITION
        };
        VkCompareOp VulkanBackend::vulkanizedCompareOp(Operations::ComparisonFunction comparisonFunction)
        {
            return VULKAN_COMPARE_OPS[static_cast<uint8_t>(comparisonFunction)];
        }

        std::vector<VkStencilOp> VULKAN_STENCIL_OPS
        {
#define DEFINITION(SlagName,VulkanName, DX12Name) VulkanName,
            STENCIL_OP_DEFINITIONS(DEFINITION)
#undef DEFINITION
        };
        VkStencilOp VulkanBackend::vulkanizedStencilOp(Operations::StencilOperation stencilOperation)
        {
            return VULKAN_STENCIL_OPS[static_cast<uint8_t>(stencilOperation)];
        }

        VkFormat VulkanBackend::vulkanizedGraphicsType(GraphicsType graphicsType)
        {
            switch(graphicsType)
            {
                case GraphicsType::UNKNOWN:
                    return VK_FORMAT_UNDEFINED;
                case GraphicsType::BOOLEAN:
                    return VK_FORMAT_R8_UINT;
                case GraphicsType::INTEGER:
                    return VK_FORMAT_R32_SINT;
                case GraphicsType::UNSIGNED_INTEGER:
                    return VK_FORMAT_R32_UINT;
                case GraphicsType::FLOAT:
                    return VK_FORMAT_R32_SFLOAT;
                case GraphicsType::DOUBLE:
                    return VK_FORMAT_R64_SFLOAT;
                case GraphicsType::VECTOR2:
                    return VK_FORMAT_R32G32_SFLOAT;
                case GraphicsType::VECTOR3:
                    return VK_FORMAT_R32G32B32_SFLOAT;
                case GraphicsType::VECTOR4:
                    return VK_FORMAT_R32G32B32A32_SFLOAT;
                case GraphicsType::BOOLEAN_VECTOR2:
                    return VK_FORMAT_R8G8_UINT;
                case GraphicsType::BOOLEAN_VECTOR3:
                    return VK_FORMAT_R8G8B8_UINT;
                case GraphicsType::BOOLEAN_VECTOR4:
                    return VK_FORMAT_R8G8B8A8_UINT;
                case GraphicsType::INTEGER_VECTOR2:
                    return VK_FORMAT_R32G32_SINT;
                case GraphicsType::INTEGER_VECTOR3:
                    return VK_FORMAT_R32G32B32_SINT;
                case GraphicsType::INTEGER_VECTOR4:
                    return VK_FORMAT_R32G32B32A32_SINT;
                case GraphicsType::UNSIGNED_INTEGER_VECTOR2:
                    return VK_FORMAT_R32G32_UINT;
                case GraphicsType::UNSIGNED_INTEGER_VECTOR3:
                    return VK_FORMAT_R32G32B32_UINT;
                case GraphicsType::UNSIGNED_INTEGER_VECTOR4:
                    return VK_FORMAT_R32G32B32A32_UINT;
                case GraphicsType::DOUBLE_VECTOR2:
                    return VK_FORMAT_R64G64_SFLOAT;
                case GraphicsType::DOUBLE_VECTOR3:
                    return VK_FORMAT_R64G64B64_SFLOAT;
                case GraphicsType::DOUBLE_VECTOR4:
                    return VK_FORMAT_R64G64B64A64_SFLOAT;
            }
            return VK_FORMAT_UNDEFINED;
        }

        VkShaderStageFlagBits VulkanBackend::vulkanizedShaderStage(ShaderStageFlags stageFlags)
        {
            switch (stageFlags)
            {
#define DEFINITION(SlagName, SlagValue, VulkanName, DXName) case ShaderStageFlags::SlagName: return VulkanName;
                SHADER_STAGE_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            throw std::invalid_argument("Invalid shader stage");
        }

        VkShaderStageFlags VulkanBackend::vulkanizedShaderFlags(ShaderStageFlags stageFlags)
        {
            VkShaderStageFlags flags = 0;
#define DEFINITION(SlagName, SlagValue, VulkanName, DXName) if((bool)(stageFlags & ShaderStageFlags::SlagName)) {flags |= VulkanName;}
            SHADER_STAGE_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            return flags;
        }

        VkFilter VulkanBackend::vulkanizedFilter(Sampler::Filter filter)
        {
            switch(filter)
            {
#define DEFINITION(slagName, vulkanName) case Sampler::Filter::slagName: return vulkanName;
                SAMPLER_FILTER_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return VK_FILTER_NEAREST;
        }

        VkSamplerMipmapMode VulkanBackend::vulkanizedMipMapMode(Sampler::Filter mipmapFilter)
        {
            switch (mipmapFilter)
            {
                case Sampler::Filter::NEAREST: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
                case Sampler::Filter::LINEAR: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
            }
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        }

        VkSamplerAddressMode VulkanBackend::vulkanizedAddressMode(Sampler::AddressMode addressMode)
        {
            switch (addressMode)
            {
#define DEFINITION(slagName, vulkanName, dx12Name) case Sampler::AddressMode::slagName: return vulkanName;
                SAMPLER_ADDRESS_MODES_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        }

        VkPresentModeKHR VulkanBackend::vulkanizedPresentMode(SwapChain::PresentMode presentMode)
        {
            switch (presentMode)
            {
#define DEFINITION(SlagValue, VulkanValue, DX12Value, AllowTearing, FrameLatency) case SwapChain::PresentMode::SlagValue: return VulkanValue;
                SLAG_PRESENT_MODE_DEFINTITIONS(DEFINITION)
    #undef DEFINITION
            }
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }

        VkCompositeAlphaFlagBitsKHR VulkanBackend::vulkanizedCompositeAlphaFlags(SwapChain::AlphaCompositing composite)
        {
            switch (composite)
            {
#define DEFINITION(SlagValue, VulkanValue, DX12Value) case SwapChain::AlphaCompositing::SlagValue: return VulkanValue;
                SLAG_SWAPCHAIN_ALPHA_MODE(DEFINITION)
    #undef DEFINITION
            }
            return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        }

        VkDescriptorType VulkanBackend::vulkanizedDescriptorType(Descriptor::Type descriptorType)
        {
            switch (descriptorType)
            {
            case slag::Descriptor::Type::SAMPLER:
                return  VK_DESCRIPTOR_TYPE_SAMPLER;
            case slag::Descriptor::Type::SAMPLED_TEXTURE:
                return  VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            case slag::Descriptor::Type::STORAGE_TEXTURE:
                return  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            case slag::Descriptor::Type::UNIFORM_TEXEL_BUFFER:
                return  VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            case slag::Descriptor::Type::STORAGE_TEXEL_BUFFER:
                return  VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            case slag::Descriptor::Type::UNIFORM_BUFFER:
                return  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case slag::Descriptor::Type::STORAGE_BUFFER:
                return  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            case slag::Descriptor::Type::ACCELERATION_STRUCTURE:
                return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            }
            throw std::runtime_error("unable to convert descriptorType");
        }

        void(* SLAG_VULKAN_DEBUG_HANDLER)(const std::string& message, SlagDebugLevel level, int32_t messageID)=nullptr;
        VkBool32 VULKAN_DEBUG_MESSENGER_CALLBACK(VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
                                                 VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
                                                 const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
                                                 void*                                            pUserData)
        {
            if(SLAG_VULKAN_DEBUG_HANDLER!= nullptr)
            {
                SlagDebugLevel level = SlagDebugLevel::SLAG_INFO;
                if(messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
                {
                    level = SlagDebugLevel::SLAG_ERROR;
                }
                else if(messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
                {
                    level = SlagDebugLevel::SLAG_WARNING;
                }
                std::string message = pCallbackData->pMessage;
                SLAG_VULKAN_DEBUG_HANDLER(message,level,pCallbackData->messageIdNumber);
            }
            return VK_FALSE;
        }

        VulkanBackend::VulkanBackend(const SlagInitInfo& initInfo)
        {
            if (initInfo.slagDebugHandler)
            {
                SLAG_VULKAN_DEBUG_HANDLER = initInfo.slagDebugHandler;
            }
            vkb::InstanceBuilder builder;
            auto inst = builder.set_app_name("Slag Application")
                              .request_validation_layers(initInfo.slagDebugHandler)
                              .set_debug_callback(VULKAN_DEBUG_MESSENGER_CALLBACK)
                              .require_api_version(1,3,0)
                               .enable_extension("VK_KHR_get_surface_capabilities2")
                                .enable_extension("VK_EXT_surface_maintenance1")
                              .build();

            if(!inst.has_value())
            {
                return;
            }
            _debugMessenger = inst->debug_messenger;
            _instance = inst.value();
            _isValid = true;
        }

        VulkanBackend::~VulkanBackend()
        {
            if (_debugMessenger != nullptr)
            {
                vkb::destroy_debug_utils_messenger(_instance,_debugMessenger);
            }
            vkDestroyInstance(_instance, nullptr);
            SLAG_VULKAN_DEBUG_HANDLER = nullptr;
        }

        void VulkanBackend::postGraphicsCardChosenSetup()
        {
            VulkanTexture::initializeChromaConverters();
        }

        void VulkanBackend::preGraphicsCardDestroyCleanup()
        {
            VulkanTexture::cleanupChromaConverters();
        }

        bool VulkanBackend::valid()
        {
            return _isValid;
        }

        std::vector<std::unique_ptr<GraphicsCard>> VulkanBackend::getGraphicsCards()
        {

            VkPhysicalDeviceVulkan13Features features1_3{.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
            features1_3.dynamicRendering = true;
            features1_3.synchronization2 = true;


            VkPhysicalDeviceVulkan12Features features1_2{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
            features1_2.bufferDeviceAddress = true;
            features1_2.descriptorIndexing = true;
            features1_2.timelineSemaphore = true;
            features1_2.drawIndirectCount = true;
            features1_2.shaderInt8 = true;
            features1_2.shaderFloat16 = true;

            VkPhysicalDeviceSamplerYcbcrConversionFeatures ycbcrFeatures{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES};
            ycbcrFeatures.samplerYcbcrConversion = true;


            VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR shaderDerivativesFeatures{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_KHR};
            shaderDerivativesFeatures.pNext = &ycbcrFeatures;
            shaderDerivativesFeatures.computeDerivativeGroupLinear = true;
            shaderDerivativesFeatures.computeDerivativeGroupQuads = true;


            VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchainFeatures{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT};
            swapchainFeatures.pNext = &shaderDerivativesFeatures;

            VkPhysicalDeviceFeatures2 features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
            features.pNext = &swapchainFeatures;

            features1_2.pNext = &swapchainFeatures;

            VkPhysicalDeviceFeatures basicFeatures{};
            basicFeatures.fillModeNonSolid = true;
            basicFeatures.wideLines = true;
            basicFeatures.largePoints = true;
            basicFeatures.depthClamp = true;
            basicFeatures.sampleRateShading = true;
            basicFeatures.alphaToOne = true;
            basicFeatures.logicOp = true;
            basicFeatures.fragmentStoresAndAtomics = true;



            vkb::PhysicalDeviceSelector selector{_instance};
            auto physicalDevices = selector.set_minimum_version(1,3)
                                            .set_required_features_13(features1_3)
                                            .set_required_features_12(features1_2)
                                            .set_required_features(basicFeatures)
                                            .add_required_extension("VK_EXT_swapchain_maintenance1")
                                            .add_required_extension("VK_EXT_custom_border_color")
                                            .add_required_extension("VK_KHR_compute_shader_derivatives")
                                            .defer_surface_initialization()
                                            .select_devices();

            std::vector<std::unique_ptr<GraphicsCard>> graphicsCards;
            if (physicalDevices.has_value())
            {
                for (auto& physicalDevice : physicalDevices.value())
                {
                    vkb::DeviceBuilder deviceBuilder{physicalDevice};
                    auto device = deviceBuilder.build();
                    if(device.has_value())
                    {
                        graphicsCards.emplace_back(std::make_unique<VulkanGraphicsCard>(_instance.instance, device.value()));
                    }
                }
            }
            return graphicsCards;
        }

        GraphicsBackend VulkanBackend::backendAPI()
        {
            return GraphicsBackend::VULKAN_GRAPHICS_BACKEND;
        }

        CommandBuffer* VulkanBackend::newCommandBuffer(GPUQueue::QueueType acceptsCommands)
        {
            return new VulkanCommandBuffer(acceptsCommands);
        }

        CommandBuffer* VulkanBackend::newSubCommandBuffer(CommandBuffer* parentBuffer)
        {
            throw std::runtime_error("Not implemented");
        }

        Semaphore* VulkanBackend::newSemaphore(uint64_t initialValue)
        {
            return new VulkanSemaphore(initialValue);
        }

        void VulkanBackend::waitFor(SemaphoreValue* values, size_t count)
        {
            VulkanSemaphore::waitFor(values, count);
        }

        Texture* VulkanBackend::newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount)
        {
            return new VulkanTexture(texelFormat, type, usageFlags, width, height, depth,mipLevels, layers, sampleCount);
        }


        Texture* VulkanBackend::newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount, void* texelData, uint64_t texelDataLength, TextureBufferMapping* mappings, uint32_t mappingCount)
        {
            return new VulkanTexture(texelFormat, type,usageFlags,width,height, depth,mipLevels,layers,sampleCount,texelData, texelDataLength,mappings,mappingCount);
        }

        Buffer* VulkanBackend::newBuffer(size_t dataSize, Buffer::Accessibility accessibility, Buffer::UsageFlags usage)
        {
            return new VulkanBuffer(dataSize, accessibility, usage);
        }

        Buffer* VulkanBackend::newBuffer(void* data, size_t dataSize, Buffer::Accessibility accessibility, Buffer::UsageFlags usage)
        {
            return new VulkanBuffer(data, dataSize, accessibility, usage);
        }

        BufferView* VulkanBackend::newBufferView(Buffer* buffer, Pixels::Format format, uint64_t offset, uint64_t size)
        {
            return new VulkanBufferView(buffer, format, offset, size);
        }

        SwapChain* VulkanBackend::newSwapChain(PlatformData platformData, uint32_t width, uint32_t height, SwapChain::PresentMode presentMode, uint8_t frameCount, Pixels::Format format, SwapChain::AlphaCompositing compositing,FrameResources*(* createResourceFunction)(uint8_t frameIndex, SwapChain* inChain),void (*swapchainRebuiltFunction)(SwapChain* swapChain))
        {
            return new VulkanSwapChain(platformData, width, height, presentMode, frameCount, format, compositing, createResourceFunction,swapchainRebuiltFunction);
        }

        Sampler* VulkanBackend::newSampler(SamplerParameters parameters)
        {
            return new VulkanSampler(parameters);
        }

        std::vector<ShaderCode::CodeLanguage> VulkanBackend::acceptedLanuages()
        {
            return std::vector<ShaderCode::CodeLanguage>
            {
                ShaderCode::CodeLanguage::SPIRV
            };
        }

        ShaderPipeline* VulkanBackend::newShaderPipeline(ShaderCode** shaders, uint32_t shaderCount, ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription, std::string(*rename)(const std::string&,uint32_t,Descriptor::Type, Descriptor::Dimension, uint32_t, uint32_t,void*), void* renameData)
        {
            return new VulkanShaderPipeline(shaders, shaderCount, properties, vertexDescription, framebufferDescription, rename, renameData);
        }

        ShaderPipeline* VulkanBackend::newShaderPipeline(const ShaderCode& computeShader, std::string(*rename)(const std::string&,uint32_t,Descriptor::Type, Descriptor::Dimension,uint32_t, uint32_t,void*), void* renameData)
        {
            return new VulkanShaderPipeline(computeShader, rename, renameData);
        }

        DescriptorPool* VulkanBackend::newDescriptorPool()
        {
            return new VulkanDescriptorPool(DescriptorPoolPageInfo{});
        }

        DescriptorPool* VulkanBackend::newDescriptorPool(const DescriptorPoolPageInfo& pageInfo)
        {
            return new VulkanDescriptorPool(pageInfo);
        }

        void VulkanBackend::setDescriptorBundleSampler(DescriptorBundle& descriptor, DescriptorIndex* index,uint32_t arrayElement, Sampler* sampler)
        {
            VkDescriptorSet descriptorSet = static_cast<VkDescriptorSet>(descriptor.gpuHandle());
            auto vIndex = static_cast<VulkanDescriptorIndex*>(index);
            auto s = static_cast<VulkanSampler*>(sampler);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.sampler = s->vulkanHandle();
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = descriptorSet;
            write.dstBinding = vIndex->binding;
            write.dstArrayElement = arrayElement;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            write.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(VulkanGraphicsCard::selected()->device(),1,&write,0, nullptr);
        }
        void VulkanBackend::setDescriptorBundleSampledTexture(DescriptorBundle& descriptor, DescriptorIndex* index, uint32_t arrayElement, Texture* texture)
        {
            VkDescriptorSet descriptorSet = static_cast<VkDescriptorSet>(descriptor.gpuHandle());
            auto vIndex = static_cast<VulkanDescriptorIndex*>(index);
            auto tex = static_cast<VulkanTexture*>(texture);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageInfo.imageView = tex->vulkanViewHandle();

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = descriptorSet;
            write.dstBinding = vIndex->binding;
            write.dstArrayElement = arrayElement;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            write.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(VulkanGraphicsCard::selected()->device(),1,&write,0, nullptr);
        }

        void VulkanBackend::setDescriptorBundleStorageTexture(DescriptorBundle& descriptor, DescriptorIndex* index, uint32_t arrayElement, Texture* texture)
        {
            VkDescriptorSet descriptorSet = static_cast<VkDescriptorSet>(descriptor.gpuHandle());
            auto vIndex = static_cast<VulkanDescriptorIndex*>(index);
            auto tex = static_cast<VulkanTexture*>(texture);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageInfo.imageView = tex->vulkanViewHandle();

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = descriptorSet;
            write.dstBinding = vIndex->binding;
            write.dstArrayElement = arrayElement;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            write.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(VulkanGraphicsCard::selected()->device(),1,&write,0, nullptr);
        }
        void VulkanBackend::setDescriptorBundleUniformTexelBuffer(DescriptorBundle& descriptor, DescriptorIndex* index, uint32_t arrayElement, BufferView* bufferView)
        {
            VkDescriptorSet descriptorSet = static_cast<VkDescriptorSet>(descriptor.gpuHandle());
            auto vIndex = static_cast<VulkanDescriptorIndex*>(index);
            auto buf = static_cast<VulkanBufferView*>(bufferView);

            auto handle = buf->vulkanHandle();

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = descriptorSet;
            write.dstBinding = vIndex->binding;
            write.dstArrayElement = arrayElement;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            write.pTexelBufferView = &handle;

            vkUpdateDescriptorSets(VulkanGraphicsCard::selected()->device(),1,&write,0, nullptr);

        }
        void VulkanBackend::setDescriptorBundleStorageTexelBuffer(DescriptorBundle& descriptor, DescriptorIndex* index, uint32_t arrayElement, BufferView* bufferView)
        {
            VkDescriptorSet descriptorSet = static_cast<VkDescriptorSet>(descriptor.gpuHandle());
            auto vIndex = static_cast<VulkanDescriptorIndex*>(index);
            auto buf = static_cast<VulkanBufferView*>(bufferView);

            auto handle = buf->vulkanHandle();

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = descriptorSet;
            write.dstBinding = vIndex->binding;
            write.dstArrayElement = arrayElement;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            write.pTexelBufferView = &handle;

            vkUpdateDescriptorSets(VulkanGraphicsCard::selected()->device(),1,&write,0, nullptr);
        }
        void VulkanBackend::setDescriptorBundleUniformBuffer(DescriptorBundle& descriptor, DescriptorIndex* index, uint32_t arrayElement, Buffer* buffer, uint64_t offset, uint64_t length)
        {
            VkDescriptorSet descriptorSet = static_cast<VkDescriptorSet>(descriptor.gpuHandle());
            auto vIndex = static_cast<VulkanDescriptorIndex*>(index);
            auto buf = static_cast<VulkanBuffer*>(buffer);

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = buf->vulkanHandle();
            bufferInfo.offset = offset;
            bufferInfo.range = length;

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = descriptorSet;
            write.dstBinding = vIndex->binding;
            write.dstArrayElement = arrayElement;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(VulkanGraphicsCard::selected()->device(),1,&write,0, nullptr);
        }
        void VulkanBackend::setDescriptorBundleStorageBuffer(DescriptorBundle& descriptor, DescriptorIndex* index, uint32_t arrayElement, Buffer* buffer, uint64_t offset, uint64_t length)
        {
            VkDescriptorSet descriptorSet = static_cast<VkDescriptorSet>(descriptor.gpuHandle());
            auto vIndex = static_cast<VulkanDescriptorIndex*>(index);
            auto buf = static_cast<VulkanBuffer*>(buffer);

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = buf->vulkanHandle();
            bufferInfo.offset = offset;
            bufferInfo.range = length;

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = descriptorSet;
            write.dstBinding = vIndex->binding;
            write.dstArrayElement = arrayElement;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(VulkanGraphicsCard::selected()->device(),1,&write,0, nullptr);
        }

        PixelFormatProperties VulkanBackend::pixelFormatProperties(Pixels::Format format)
        {
            auto nativeFormat = VulkanBackend::vulkanizedFormat(format);
            VkFormatProperties2 formatProperties{.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2};
            vkGetPhysicalDeviceFormatProperties2(VulkanGraphicsCard::selected()->physicalDevice(),nativeFormat.format,&formatProperties);
            PixelFormatProperties properties{};
            VkFormatFeatureFlags2 features = 0;
            Texture::UsageFlags usage =static_cast<Texture::UsageFlags>(0);

            if (formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT && formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT)
            {
                properties.tiling = PixelFormatProperties::Tiling::OPTIMIZED;
                features = formatProperties.formatProperties.optimalTilingFeatures;
            }
            else if (formatProperties.formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT && formatProperties.formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT)
            {
                properties.tiling = PixelFormatProperties::Tiling::LINEAR;
                features = formatProperties.formatProperties.optimalTilingFeatures;
            }
            else
            {
                properties.tiling = PixelFormatProperties::Tiling::UNSUPPORTED;
                return properties;
            }
            bool hasUsage = false;
            if (features & VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT)
            {
                usage |= Texture::UsageFlags::SAMPLED_IMAGE;
                hasUsage = true;
            }
            if (features & VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT )
            {
                usage |= Texture::UsageFlags::STORAGE;
                hasUsage = true;
            }
            if (features & VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT)
            {
                usage |= Texture::UsageFlags::RENDER_TARGET_ATTACHMENT;
                hasUsage = true;
            }
            if (features & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                usage |= Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT;
                hasUsage = true;
            }
            properties.validUsageFlags = usage;
            if (!hasUsage)
            {
                properties.tiling = PixelFormatProperties::Tiling::UNSUPPORTED;
                return properties;
            }
            if (features & VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
            {
                properties.linearFilteringCapable = true;
            }
            if (features & VK_FORMAT_FEATURE_2_BLIT_SRC_BIT )
            {
                properties.blitSource = true;
            }
            if (features & VK_FORMAT_FEATURE_2_BLIT_DST_BIT )
            {
                properties.blitDestination = true;
            }


            return properties;
        }

        vkb::Instance VulkanBackend::vulkanInstance()
        {
            return _instance;
        }
    } // vulkan
} // slag
