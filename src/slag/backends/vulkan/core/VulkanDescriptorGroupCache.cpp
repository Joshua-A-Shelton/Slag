#include "VulkanDescriptorGroupCache.h"

#include "VulkanDescriptorGroup.h"
#include "VulkanGraphicsCard.h"
#include "slag/backends/vulkan/VulkanBackend.h"

namespace slag
{
    namespace vulkan
    {
        VkDescriptorSetLayout VulkanDescriptorGroupCache::getLayout(VulkanDescriptorGroup& group)
        {
            if(group.descriptorCount() == 0)
            {
                return nullptr;
            }
            std::lock_guard<std::mutex> lock(_cacheMutex);

            auto id = group.shape();
            auto layout = _cachedLayouts.find(id);
            if(layout == _cachedLayouts.end())
            {
                CachedLayout cached{};
                cached.instanceCount = 1;

                std::vector<VkDescriptorSetLayoutBinding> bindings(group.descriptorCount());
                uint32_t actualBindings = 0;
                for(size_t i=0; i< group.descriptorCount(); i++)
                {
                    auto& descriptor = group[i];
                    if (descriptor.shape().type == Descriptor::Type::UNKNOWN)
                    {
                        continue;
                    }
                    VkDescriptorSetLayoutBinding& binding = bindings[actualBindings];
                    binding.descriptorType = VulkanBackend::vulkanizedDescriptorType(descriptor.shape().type);
                    binding.descriptorCount = descriptor.shape().arrayDepth;
                    binding.binding = descriptor.shape().binding;
                    binding.stageFlags = VulkanBackend::vulkanizedShaderFlags(descriptor.shape().visibleStages);
                    actualBindings++;
                }
                VkDescriptorSetLayoutCreateInfo layoutInfo{};
                layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layoutInfo.bindingCount = actualBindings;
                layoutInfo.pBindings = bindings.data();

                auto result = vkCreateDescriptorSetLayout(VulkanGraphicsCard::selected()->device(), &layoutInfo, nullptr, &cached.layout);
                if(result != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create descriptor set layout");
                }
                _cachedLayouts.insert(std::make_pair(id,cached));
                return cached.layout;
            }
            else
            {
                layout->second.instanceCount++;
                return layout->second.layout;
            }
        }

        void VulkanDescriptorGroupCache::removeInstance(VulkanDescriptorGroup& group)
        {
            if(group.descriptorCount() == 0)
            {
                return;
            }

            std::lock_guard<std::mutex> lock(_cacheMutex);

            auto hash = group.shape();
            auto layout = _cachedLayouts.find(hash);
            if(layout != _cachedLayouts.end())
            {
                layout->second.instanceCount--;
                if(layout->second.instanceCount == 0)
                {
                    vkDestroyDescriptorSetLayout(VulkanGraphicsCard::selected()->device(),layout->second.layout, nullptr);
                    _cachedLayouts.erase(hash);
                }
            }
        }
    } // vulkan
} // slag
