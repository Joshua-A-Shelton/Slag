#include "VulkanDescriptorGroupCache.h"

#include "VulkanDescriptorGroup.h"
#include "VulkanGraphicsCard.h"
#include "slag/backends/vulkan/VulkanBackend.h"

namespace slag
{
    namespace vulkan
    {
        VkDescriptorSetLayout VulkanDescriptorGroupCache::getLayout(VulkanDescriptorGroup::Shape& shape)
        {
            if(shape._descriptorShapes.size() == 0)
            {
                return nullptr;
            }
            std::lock_guard<std::mutex> lock(_cacheMutex);

            auto layout = _cachedLayouts.find(shape);
            if(layout == _cachedLayouts.end())
            {
                CachedLayout cached{};
                cached.instanceCount = 1;

                std::vector<VkDescriptorSetLayoutBinding> bindings(shape._descriptorShapes.size());
                uint32_t actualBindings = 0;
                for(size_t i=0; i< shape._descriptorShapes.size(); i++)
                {
                    auto& descriptor = shape._descriptorShapes[i];
                    if (descriptor.type == Descriptor::Type::UNKNOWN)
                    {
                        continue;
                    }
                    VkDescriptorSetLayoutBinding& binding = bindings[actualBindings];
                    binding.descriptorType = VulkanBackend::vulkanizedDescriptorType(descriptor.type);
                    binding.descriptorCount = descriptor.arrayDepth;
                    binding.binding = i;//this is an assumption that *should* be true, it's possible we didn't set ourselves up correctly though and for this to be wrong
                    binding.stageFlags = VulkanBackend::vulkanizedShaderFlags(descriptor.visibleStages);
                    actualBindings++;
                }
                VkDescriptorSetLayoutCreateInfo layoutInfo{};
                layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layoutInfo.bindingCount = actualBindings;
                layoutInfo.pBindings = bindings.data();
                layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

                auto result = vkCreateDescriptorSetLayout(VulkanGraphicsCard::selected()->device(), &layoutInfo, nullptr, &cached.layout);
                if(result != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create descriptor set layout");
                }
                _cachedLayouts.insert(std::make_pair(shape,cached));
                return cached.layout;
            }
            else
            {
                layout->second.instanceCount++;
                return layout->second.layout;
            }
        }

        void VulkanDescriptorGroupCache::removeInstance(VulkanDescriptorGroup::Shape& shape)
        {
            if(shape._descriptorShapes.size() == 0)
            {
                return;
            }

            std::lock_guard<std::mutex> lock(_cacheMutex);

            auto layout = _cachedLayouts.find(shape);
            if(layout != _cachedLayouts.end())
            {
                layout->second.instanceCount--;
                if(layout->second.instanceCount == 0)
                {
                    vkDestroyDescriptorSetLayout(VulkanGraphicsCard::selected()->device(),layout->second.layout, nullptr);
                    _cachedLayouts.erase(shape);
                }
            }
        }
    } // vulkan
} // slag
