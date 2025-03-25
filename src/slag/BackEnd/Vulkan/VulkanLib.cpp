#include "VulkanLib.h"
#include "VkBootstrap.h"
#include "VulkanSemaphore.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanSwapchain.h"
#include "VulkanQueue.h"
#include "VulkanTexture.h"
#include "Extensions.h"
#include "VulkanSampler.h"
#include "VulkanDescriptorGroup.h"
#include "VulkanShaderPipeline.h"
#include "VulkanDescriptorPool.h"

namespace slag
{
    namespace vulkan
    {

        void(* SLAG_VULKAN_DEBUG_HANDLER)(std::string& message, SlagInitDetails::DebugLevel level, int32_t messageID) = nullptr;
        VkBool32 VULKAN_DEBUG_MESSENGER_CALLBACK(VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
                                                 VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
                                                 const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
                                                 void*                                            pUserData)
        {
            if(SLAG_VULKAN_DEBUG_HANDLER!= nullptr)
            {
                SlagInitDetails::DebugLevel level = SlagInitDetails::DebugLevel::SLAG_MESSAGE;
                if(messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
                {
                    level = SlagInitDetails::DebugLevel::SLAG_ERROR;
                }
                else if(messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
                {
                    level = SlagInitDetails::DebugLevel::SLAG_WARNING;
                }
                std::string message = pCallbackData->pMessage;
                SLAG_VULKAN_DEBUG_HANDLER(message,level,pCallbackData->messageIdNumber);
            }
            return VK_FALSE;
        }

        VulkanLib* VulkanLib::initialize(const SlagInitDetails& details)
        {
            if(details.debug)
            {
                SLAG_VULKAN_DEBUG_HANDLER = details.slagDebugHandler;
            }

            vkb::InstanceBuilder builder;
            auto inst = builder.set_app_name("Slag Application")
                               .request_validation_layers(details.debug)
                               //.use_default_debug_messenger()
                               .set_debug_callback(VULKAN_DEBUG_MESSENGER_CALLBACK)
                               .require_api_version(1,3,0)
                               .build();

            if(!inst.has_value())
            {
                return nullptr;
            }
            auto instance = inst->instance;
            auto debugMessenger = inst->debug_messenger;

            VkPhysicalDeviceVulkan13Features features1_3{.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
            features1_3.dynamicRendering = true;
            features1_3.synchronization2 = true;


            VkPhysicalDeviceVulkan12Features features1_2{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
            features1_2.bufferDeviceAddress = true;
            features1_2.descriptorIndexing = true;
            features1_2.timelineSemaphore = true;

            vkb::PhysicalDeviceSelector selector{inst.value()};
            auto physicalDevice = selector.set_minimum_version(1,3)
                                          .set_required_features_13(features1_3)
                                          .set_required_features_12(features1_2)
                                          .add_required_extension("VK_EXT_swapchain_maintenance1")
                                          .add_required_extension("VK_EXT_custom_border_color")
                                          .defer_surface_initialization()
                                          .select();
            if(!physicalDevice.has_value())
            {
                return nullptr;
            }
            vkb::DeviceBuilder deviceBuilder{physicalDevice.value()};
            auto device = deviceBuilder.build();
            if(!device.has_value())
            {
                return nullptr;
            }
            auto card = new VulkanGraphicsCard(instance,device.value());
            Extensions::mapExtensions(card->device());

            mapFlags();

            return new VulkanLib(instance,debugMessenger,card);

        }

        void VulkanLib::cleanup(lib::BackEndLib* library)
        {
            if(get())
            {
                delete library;
            }
            SLAG_VULKAN_DEBUG_HANDLER = nullptr;
        }

        void VulkanLib::mapFlags()
        {
#define DEFINITION(slagName, vulkanName, directXName) TextureUsageFlags::set##slagName##Value(static_cast<int>(vulkanName));
            TEXTURE_USAGE_DEFINITIONS(DEFINITION)
#undef DEFINITION

#define DEFINITION(slagName, vulkanName, directXName) BarrierAccessFlags::set##slagName##Value(static_cast<int>(vulkanName));
            MEMORY_BARRIER_ACCESS_DEFINTITIONS(DEFINITION)
#undef DEFINITION

#define DEFINITION(slagName, vulkanName, directXName) PipelineStageFlags::set##slagName##Value(static_cast<int>(vulkanName));
            MEMORY_PIPELINE_STAGE_DEFINITIONS(DEFINITION)
#undef DEFINITION

#define DEFINITION(slagName, vulkanName, directXName) ShaderStageFlags::set##slagName##Value(static_cast<int>(vulkanName));
            SHADER_STAGE_DEFINTITIONS(DEFINITION)
#undef DEFINITION

        }

        VkInstance VulkanLib::instance()
        {
            return _instance;
        }

        VulkanLib* VulkanLib::get()
        {
            return static_cast<VulkanLib*>(lib::BackEndLib::get());
        }

        VulkanGraphicsCard* VulkanLib::card()
        {
            return get()->vulkanGraphicsCard();
        }

        VulkanizedFormat VulkanLib::format(Pixels::Format format)
        {
            switch (format)
            {
#define DEFINITION(SlagName, DxName, VulkanName, VkImageAspectFlags, VkComponentSwizzle_r, VkComponentSwizzle_g, VkComponentSwizzle_b, VkComponentSwizzle_a, totalBits) case Pixels::SlagName: return {.format = VulkanName, .mapping = {.r = VkComponentSwizzle_r, .g = VkComponentSwizzle_g, .b = VkComponentSwizzle_b, .a = VkComponentSwizzle_a} };
                TEXTURE_FORMAT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }

            return VulkanizedFormat{};
        }

        VkFormat VulkanLib::graphicsType(GraphicsTypes::GraphicsType type)
        {
            switch(type)
            {
                case GraphicsTypes::GraphicsType::UNKNOWN:
                    return VK_FORMAT_UNDEFINED;
                case GraphicsTypes::GraphicsType::BOOLEAN:
                    return VK_FORMAT_R8_UINT;
                case GraphicsTypes::GraphicsType::INTEGER:
                    return VK_FORMAT_R32_SINT;
                case GraphicsTypes::GraphicsType::UNSIGNED_INTEGER:
                    return VK_FORMAT_R32_UINT;
                case GraphicsTypes::GraphicsType::FLOAT:
                    return VK_FORMAT_R32_SFLOAT;
                case GraphicsTypes::GraphicsType::DOUBLE:
                    return VK_FORMAT_R64_SFLOAT;
                case GraphicsTypes::GraphicsType::VECTOR2:
                    return VK_FORMAT_R32G32_SFLOAT;
                case GraphicsTypes::GraphicsType::VECTOR3:
                    return VK_FORMAT_R32G32B32_SFLOAT;
                case GraphicsTypes::GraphicsType::VECTOR4:
                    return VK_FORMAT_R32G32B32A32_SFLOAT;
                case GraphicsTypes::GraphicsType::BOOLEAN_VECTOR2:
                    return VK_FORMAT_R8G8_UINT;
                case GraphicsTypes::GraphicsType::BOOLEAN_VECTOR3:
                    return VK_FORMAT_R8G8B8_UINT;
                case GraphicsTypes::GraphicsType::BOOLEAN_VECTOR4:
                    return VK_FORMAT_R8G8B8A8_UINT;
                case GraphicsTypes::GraphicsType::INTEGER_VECTOR2:
                    return VK_FORMAT_R32G32_SINT;
                case GraphicsTypes::GraphicsType::INTEGER_VECTOR3:
                    return VK_FORMAT_R32G32B32_SINT;
                case GraphicsTypes::GraphicsType::INTEGER_VECTOR4:
                    return VK_FORMAT_R32G32B32A32_SINT;
                case GraphicsTypes::GraphicsType::UNSIGNED_INTEGER_VECTOR2:
                    return VK_FORMAT_R32G32_UINT;
                case GraphicsTypes::GraphicsType::UNSIGNED_INTEGER_VECTOR3:
                    return VK_FORMAT_R32G32B32_UINT;
                case GraphicsTypes::GraphicsType::UNSIGNED_INTEGER_VECTOR4:
                    return VK_FORMAT_R32G32B32A32_UINT;
                case GraphicsTypes::GraphicsType::DOUBLE_VECTOR2:
                    return VK_FORMAT_R64G64_SFLOAT;
                case GraphicsTypes::GraphicsType::DOUBLE_VECTOR3:
                    return VK_FORMAT_R64G64B64_SFLOAT;
                case GraphicsTypes::GraphicsType::DOUBLE_VECTOR4:
                    return VK_FORMAT_R64G64B64A64_SFLOAT;
            }
            return VK_FORMAT_UNDEFINED;
        }

        VkImageLayout VulkanLib::layout(Texture::Layout layout)
        {
            switch(layout)
            {
#define DEFINITION(slagName, vulkanName, directXName, directXResourceName) case Texture::slagName: return vulkanName;
                TEXTURE_LAYOUT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return VK_IMAGE_LAYOUT_UNDEFINED;
        }

        VkImageType VulkanLib::imageType(Texture::Type imageType)
        {
            switch (imageType)
            {
                case Texture::Type::TEXTURE_1D:
                    return VK_IMAGE_TYPE_1D;
                case Texture::Type::TEXTURE_2D:
                case Texture::Type::CUBE_MAP:
                    return VK_IMAGE_TYPE_2D;
                case Texture::Type::TEXTURE_3D:
                    return VK_IMAGE_TYPE_3D;
            }
            return VK_IMAGE_TYPE_2D;
        }

        VkImageViewType VulkanLib::viewType(Texture::Type textureType, size_t layerCount)
        {
            if(layerCount > 1)
            {
                switch (textureType)
                {
                    case Texture::TEXTURE_1D:
                        return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
                    case Texture::TEXTURE_2D:
                        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                    case Texture::TEXTURE_3D:
                        return VK_IMAGE_VIEW_TYPE_3D;
                    case Texture::CUBE_MAP:
                        if(layerCount != 6)
                        {
                            throw std::runtime_error("cannot have cubemap with non 6 sided layer count");
                        }
                        return VK_IMAGE_VIEW_TYPE_CUBE;
                }
            }
            else
            {
                switch (textureType)
                {
                    case Texture::TEXTURE_1D:
                        return VK_IMAGE_VIEW_TYPE_1D;
                    case Texture::TEXTURE_2D:
                        return VK_IMAGE_VIEW_TYPE_2D;
                    case Texture::TEXTURE_3D:
                        return VK_IMAGE_VIEW_TYPE_3D;
                    case Texture::CUBE_MAP:
                        throw std::runtime_error("cannot have cubemap with non 6 sided layer count");
                }
            }
            return VK_IMAGE_VIEW_TYPE_2D;
        }

        VkFilter VulkanLib::filter(Sampler::Filter filter)
        {
            switch (filter)
            {
#define DEFINITION(slagName, vulkanName) case Sampler::slagName: return vulkanName;
                SAMPLER_FILTER_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return VK_FILTER_LINEAR;
        }

        VkSamplerMipmapMode VulkanLib::mipMapMode(Sampler::Filter filter)
        {
            switch (filter)
            {
                case Sampler::Filter::NEAREST:
                    return VK_SAMPLER_MIPMAP_MODE_NEAREST;
                case Sampler::Filter::LINEAR:
                    return VK_SAMPLER_MIPMAP_MODE_LINEAR;
            }
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        }

        VkSamplerAddressMode VulkanLib::addressMode(Sampler::AddressMode addressMode)
        {
            switch (addressMode)
            {
#define DEFINITION(slagName, vulkanName, dx12Name) case Sampler::slagName: return vulkanName;
                SAMPLER_ADDRESS_MODES_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }

        VkCompareOp VulkanLib::compareOp(Operations::ComparisonFunction comparisonFunction)
        {
            switch (comparisonFunction)
            {
#define DEFINITION(slagName, vulkanName, dx12Name) case Operations::slagName: return vulkanName;
                COMPARISON_FUNCTION(DEFINITION)
#undef DEFINITION
            }
            return VK_COMPARE_OP_NEVER;
        }

        VkDescriptorType VulkanLib::descriptorType(slag::Descriptor::DescriptorType descriptorType)
        {
            switch (descriptorType)
            {
                case slag::Descriptor::DescriptorType::SAMPLER:
                    return  VK_DESCRIPTOR_TYPE_SAMPLER;
                case slag::Descriptor::DescriptorType::SAMPLED_TEXTURE:
                    return  VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                case slag::Descriptor::DescriptorType::SAMPLER_AND_TEXTURE:
                    return  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                case slag::Descriptor::DescriptorType::STORAGE_TEXTURE:
                    return  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                case slag::Descriptor::DescriptorType::UNIFORM_TEXEL_BUFFER:
                    return  VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                case slag::Descriptor::DescriptorType::STORAGE_TEXEL_BUFFER:
                    return  VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                case slag::Descriptor::DescriptorType::UNIFORM_BUFFER:
                    return  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                case slag::Descriptor::DescriptorType::STORAGE_BUFFER:
                    return  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                case slag::Descriptor::DescriptorType::INPUT_ATTACHMENT:
                    return  VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                case slag::Descriptor::DescriptorType::ACCELERATION_STRUCTURE:
                    return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            }
            throw std::runtime_error("unable to convert descriptorType");
        }

        VkPolygonMode VulkanLib::polygonMode(RasterizationState::DrawMode mode)
        {
            switch (mode)
            {
                case RasterizationState::DrawMode::FACE:
                    return VK_POLYGON_MODE_FILL;
                case RasterizationState::DrawMode::EDGE:
                    return VK_POLYGON_MODE_LINE;
                case RasterizationState::DrawMode::VERTEX:
                    return VK_POLYGON_MODE_POINT;
            }
            return VK_POLYGON_MODE_FILL;
        }

        VkCullModeFlags VulkanLib::cullMode(RasterizationState::CullOptions mode)
        {
            switch (mode)
            {
                case RasterizationState::CullOptions::NONE:
                   return VK_CULL_MODE_NONE;
                case RasterizationState::CullOptions::FRONT_FACING:
                    return VK_CULL_MODE_FRONT_BIT;
                case RasterizationState::CullOptions::BACK_FACING:
                    return VK_CULL_MODE_BACK_BIT;
            }
            return VK_CULL_MODE_NONE;
        }

        VkFrontFace VulkanLib::frontFace(RasterizationState::FrontFacing facing)
        {
            switch(facing)
            {
                case RasterizationState::FrontFacing::CLOCKWISE:
                    return VK_FRONT_FACE_CLOCKWISE;
                case RasterizationState::FrontFacing::COUNTER_CLOCKWISE:
                    return VK_FRONT_FACE_COUNTER_CLOCKWISE;
            }
            return VK_FRONT_FACE_CLOCKWISE;
        }

        VkBlendFactor VulkanLib::blendFactor(Operations::BlendFactor factor)
        {
            switch (factor)
            {
#define DEFINITION(SlagName, VulkanName, DXName) case Operations::BlendFactor::SlagName: return VulkanName;
                BLEND_FACTOR_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return VK_BLEND_FACTOR_ZERO;
        }

        VkBlendOp VulkanLib::blendOp(Operations::BlendOperation op)
        {
            switch(op)
            {
#define DEFINITION(SlagName, VulkanName, DXName) case Operations::BlendOperation::SlagName: return VulkanName;
                BLEND_OP_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return VK_BLEND_OP_ADD;
        }

        VkColorComponentFlags VulkanLib::colorComponents(Color::ComponentFlags componentFlags)
        {
            VkColorComponentFlags colorFlags = 0;
            if(componentFlags & Color::RED_COMPONENT)
            {
                colorFlags|= VK_COLOR_COMPONENT_R_BIT;
            }
            if(componentFlags & Color::GREEN_COMPONENT)
            {
                colorFlags|= VK_COLOR_COMPONENT_G_BIT;
            }
            if(componentFlags & Color::BLUE_COMPONENT)
            {
                colorFlags|= VK_COLOR_COMPONENT_B_BIT;
            }
            if(componentFlags & Color::ALPHA_COMPONENT)
            {
                colorFlags|= VK_COLOR_COMPONENT_A_BIT;
            }
            return colorFlags;
        }

        VkLogicOp VulkanLib::logicOp(Operations::LogicalOperation op)
        {
            switch (op)
            {
#define DEFINITION(SlagName,VulkanName,DX12Name) case Operations::LogicalOperation::SlagName: return VulkanName;
                FRAMEBUFFER_LOGICAL_OP_DEFINITIONS(DEFINITION)
#undef DEFINITION
            }
            return VK_LOGIC_OP_NO_OP;
        }

        VkStencilOp VulkanLib::stencilOp(Operations::StencilOperation op)
        {
            switch(op)
            {
#define DEFINITION(SlagName,VulkanName,DX12Name) case Operations::StencilOperation::SlagName: return VulkanName;
                STENCIL_OP_DEFINITIONS(DEFINITION)
#undef DEFINITION
            }
            return VK_STENCIL_OP_KEEP;
        }

        VkIndexType VulkanLib::indexType(Buffer::IndexSize iSize)
        {
            if(iSize == Buffer::UINT16)
            {
                return VK_INDEX_TYPE_UINT16;
            }
            return VK_INDEX_TYPE_UINT32;
        }

        VkClearValue VulkanLib::clearValue(slag::ClearValue clearValue)
        {
            return std::bit_cast<VkClearValue>(clearValue);
        }

        VulkanLib::VulkanLib(VkInstance instance, VkDebugUtilsMessengerEXT messenger, VulkanGraphicsCard* card)
        {
            _instance = instance;
            _debugMessenger = messenger;
            _graphicsCard = card;
        }

        VulkanLib::~VulkanLib()
        {
            if(_debugMessenger)
            {
                vkb::destroy_debug_utils_messenger(_instance,_debugMessenger);
            }
            if(graphicsCard())
            {
                delete _graphicsCard;
            }
            if(instance())
            {
                vkDestroyInstance(_instance, nullptr);
            }
        }

        BackEnd VulkanLib::identifier()
        {
            return VULKAN;
        }

        GraphicsCard* VulkanLib::graphicsCard()
        {
            return _graphicsCard;
        }
        VulkanGraphicsCard* VulkanLib::vulkanGraphicsCard()
        {
            return _graphicsCard;
        }

        Semaphore* VulkanLib::newSemaphore(uint64_t startingValue)
        {
            return new VulkanSemaphore(startingValue);
        }

        void VulkanLib::waitFor(SemaphoreValue* values, size_t count)
        {
            VulkanSemaphore::waitFor(values,count);
        }

        Swapchain* VulkanLib::newSwapchain(PlatformData platformData, uint32_t width, uint32_t height, uint8_t backBuffers, Swapchain::PresentMode mode, Pixels::Format imageFormat,FrameResources* (*createResourceFunction)(size_t frameIndex, Swapchain* inChain))
        {
            return new VulkanSwapchain(platformData,width,height,backBuffers,mode, imageFormat,createResourceFunction);
        }

        Texture* VulkanLib::newTexture(void** texelDataArray, size_t texelDataCount, size_t dataSize, Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, TextureUsage usage, Texture::Layout initializedLayout)
        {
            return new VulkanTexture(texelDataArray,texelDataCount,dataSize,dataFormat,type,width,height,mipLevels,std::bit_cast<VkImageUsageFlags>(usage),initializedLayout,false);
        }

        Texture* VulkanLib::newTexture(Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layers, uint8_t sampleCount, TextureUsage usage)
        {
            return new VulkanTexture(dataFormat,type,width,height,mipLevels,layers,sampleCount,std::bit_cast<VkImageUsageFlags>(usage),false);
        }

        CommandBuffer* VulkanLib::newCommandBuffer(GpuQueue::QueueType acceptsCommands)
        {
            uint32_t family = VulkanLib::_graphicsCard->graphicsQueueFamily();
            switch (acceptsCommands)
            {
                case GpuQueue::GRAPHICS:
                    family = VulkanLib::_graphicsCard->graphicsQueueFamily();
                    break;
                case GpuQueue::TRANSFER:
                    family = VulkanLib::_graphicsCard->transferQueueFamily();
                    break;
                case GpuQueue::COMPUTE:
                    family = VulkanLib::_graphicsCard->computeQueueFamily();
                    break;

            }
            return new VulkanCommandBuffer(family);
        }

        Buffer* VulkanLib::newBuffer(void* data, size_t dataSize, Buffer::Accessibility accessibility, Buffer::Usage usage)
        {
            VkBufferUsageFlags usageFlags = 0;

            if(usage & Buffer::Usage::VERTEX_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            }
            if(usage & Buffer::Usage::INDEX_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            }
            if (usage & Buffer::Usage::UNIFORM_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            }
            if(usage & Buffer::Usage::STORAGE_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }
            if (usage & Buffer::Usage::UNIFORM_TEXEL_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
            }
            if (usage & Buffer::Usage::STORAGE_TEXEL_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
            }
            if(usage & Buffer::Usage::INDIRECT_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            }

            return new VulkanBuffer(data,dataSize,accessibility,usageFlags, false);
        }

        Buffer* VulkanLib::newBuffer(size_t bufferSize, Buffer::Accessibility accessibility, Buffer::Usage usage)
        {
            VkBufferUsageFlags usageFlags = 0;

            if(usage & Buffer::Usage::VERTEX_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            }
            if(usage & Buffer::Usage::INDEX_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            }
            if (usage & Buffer::Usage::UNIFORM_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            }
            if(usage & Buffer::Usage::STORAGE_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }
            if (usage & Buffer::Usage::UNIFORM_TEXEL_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
            }
            if (usage & Buffer::Usage::STORAGE_TEXEL_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
            }
            if(usage & Buffer::Usage::INDIRECT_BUFFER)
            {
                usageFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            }

            return new VulkanBuffer(bufferSize,accessibility,usageFlags, false);
        }

        Sampler* VulkanLib::newSampler(Sampler::Filter minFilter, Sampler::Filter magFilter, Sampler::Filter mipMapFilter, Sampler::AddressMode u, Sampler::AddressMode v, Sampler::AddressMode w, float mipLODBias, bool enableAnisotrophy, uint8_t maxAnisotrophy,Operations::ComparisonFunction comparisonFunction, Color borderColor, float minLOD, float maxLOD)
        {
            return new VulkanSampler(minFilter,magFilter,mipMapFilter,u,v,w,mipLODBias,enableAnisotrophy,maxAnisotrophy,comparisonFunction,minLOD,maxLOD,borderColor, false);
        }

        DescriptorGroup* VulkanLib::newDescriptorGroup(Descriptor* descriptors, size_t descriptorCount)
        {
            return new VulkanDescriptorGroup(descriptors,descriptorCount);
        }

        ShaderPipeline* VulkanLib::newShaderPipeline(ShaderModule* modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, ShaderProperties& properties, VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription)
        {
            return new VulkanShaderPipeline(modules, moduleCount, descriptorGroups, descriptorGroupCount, properties, vertexDescription, frameBufferDescription, false);
        }

        ShaderPipeline* VulkanLib::newShaderPipeline(ShaderModule** modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, ShaderProperties& properties,
                                                     VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription)
        {
            return new VulkanShaderPipeline(modules, moduleCount, descriptorGroups, descriptorGroupCount, properties, vertexDescription, frameBufferDescription, false);;
        }

        DescriptorPool*
        VulkanLib::newDescriptorPool(const DescriptorPoolPageInfo& pageInfo)
        {
            return new VulkanDescriptorPool(pageInfo);
        }

        void VulkanLib::setSampler(void* handle, uint32_t binding, uint32_t arrayElement, Sampler* sampler, Texture::Layout layout)
        {
            VkDescriptorSet descriptorSet = static_cast<VkDescriptorSet>(handle);
            auto s = static_cast<VulkanSampler*>(sampler);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.sampler = s->vulkanSampler();
            imageInfo.imageLayout = VulkanLib::layout(layout);

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = descriptorSet;
            write.dstBinding = binding;
            write.dstArrayElement = arrayElement;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            write.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(card()->device(),1,&write,0, nullptr);

        }

        void VulkanLib::setSampledTexture(void* handle, uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout)
        {
            VkDescriptorSet descriptorSet = static_cast<VkDescriptorSet>(handle);
            auto tex = static_cast<VulkanTexture*>(texture);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VulkanLib::layout(layout);
            imageInfo.imageView = tex->view();

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = descriptorSet;
            write.dstBinding = binding;
            write.dstArrayElement = arrayElement;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            write.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(card()->device(),1,&write,0, nullptr);
        }

        void VulkanLib::setSamplerAndTexture(void* handle, uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout, Sampler* sampler)
        {
            VkDescriptorSet descriptorSet = static_cast<VkDescriptorSet>(handle);
            auto tex = static_cast<VulkanTexture*>(texture);
            auto s = static_cast<VulkanSampler*>(sampler);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.sampler = s->vulkanSampler();
            imageInfo.imageLayout = VulkanLib::layout(layout);
            imageInfo.imageView = tex->view();

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = descriptorSet;
            write.dstBinding = binding;
            write.dstArrayElement = arrayElement;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(card()->device(),1,&write,0, nullptr);
        }

        void VulkanLib::setStorageTexture(void* handle, uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout)
        {
            VkDescriptorSet descriptorSet = static_cast<VkDescriptorSet>(handle);
            auto tex = static_cast<VulkanTexture*>(texture);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VulkanLib::layout(layout);
            imageInfo.imageView = tex->view();

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = descriptorSet;
            write.dstBinding = binding;
            write.dstArrayElement = arrayElement;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            write.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(card()->device(),1,&write,0, nullptr);
        }

        void VulkanLib::setUniformTexelBuffer(void* handle, uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
        {
            throw std::runtime_error("VulkanLib::setUniformTexelBuffer is not implemented");
        }

        void VulkanLib::setStorageTexelBuffer(void* handle, uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
        {
            throw std::runtime_error("VulkanLib::setStorageTexelBuffer is not implemented");
        }

        void VulkanLib::setUniformBuffer(void* handle, uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
        {
            VkDescriptorSet descriptorSet = static_cast<VkDescriptorSet>(handle);
            auto buf = static_cast<VulkanBuffer*>(buffer);

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = buf->underlyingBuffer();
            bufferInfo.offset = offset;
            bufferInfo.range = length;

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = descriptorSet;
            write.dstBinding = binding;
            write.dstArrayElement = arrayElement;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(card()->device(),1,&write,0, nullptr);
        }

        void VulkanLib::setStorageBuffer(void* handle, uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
        {
            VkDescriptorSet descriptorSet = static_cast<VkDescriptorSet>(handle);
            auto buf = static_cast<VulkanBuffer*>(buffer);

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = buf->underlyingBuffer();
            bufferInfo.offset = offset;
            bufferInfo.range = length;

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = descriptorSet;
            write.dstBinding = binding;
            write.dstArrayElement = arrayElement;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(card()->device(),1,&write,0, nullptr);
        }

        void VulkanLib::setInputAttachment(void* handle,uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout)
        {
            VkDescriptorSet descriptorSet = static_cast<VkDescriptorSet>(handle);
            auto tex = static_cast<VulkanTexture*>(texture);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VulkanLib::layout(layout);
            imageInfo.imageView = tex->view();

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = descriptorSet;
            write.dstBinding = binding;
            write.dstArrayElement = arrayElement;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            write.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(card()->device(),1,&write,0, nullptr);
        }

    } // vulkan
} // slag