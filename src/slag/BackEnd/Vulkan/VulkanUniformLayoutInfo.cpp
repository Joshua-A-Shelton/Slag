#include "VulkanUniformLayoutInfo.h"

namespace slag
{
    namespace vulkan
    {
        VulkanUniformLayoutInfo::VulkanUniformLayoutInfo(std::vector<VkDescriptorSetLayoutBinding> &&bindings)
        {
            _bindings = bindings;
            _layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            _layoutInfo.flags = 0;
            _layoutInfo.bindingCount = _bindings.size();
            _layoutInfo.pBindings = _bindings.data();
        }

        void VulkanUniformLayoutInfo::copy(const VulkanUniformLayoutInfo &from)
        {
            _bindings = from._bindings;
            _layoutInfo = from._layoutInfo;
        }

        void VulkanUniformLayoutInfo::move(VulkanUniformLayoutInfo &&from)
        {
            _layoutInfo = from._layoutInfo;
            _bindings = std::move(from._bindings);
        }

        VulkanUniformLayoutInfo::VulkanUniformLayoutInfo(VulkanUniformLayoutInfo &&from)
        {
            move(std::move(from));
        }

        VulkanUniformLayoutInfo &VulkanUniformLayoutInfo::operator=(VulkanUniformLayoutInfo &&from)
        {
            move(std::move(from));
            return *this;
        }

        VulkanUniformLayoutInfo::VulkanUniformLayoutInfo(const VulkanUniformLayoutInfo &from)
        {
            copy(from);
        }

        VulkanUniformLayoutInfo &VulkanUniformLayoutInfo::operator=(VulkanUniformLayoutInfo &from)
        {
            copy(from);
            return *this;
        }

        VkDescriptorSetLayoutCreateInfo &VulkanUniformLayoutInfo::layoutInfo()
        {
            return _layoutInfo;
        }

        const std::vector<VkDescriptorSetLayoutBinding> &VulkanUniformLayoutInfo::bindings()
        {
            return _bindings;
        }

        size_t VulkanUniformLayoutInfo::hash() const
        {
            using std::size_t;
            using std::hash;

            size_t result = hash<size_t>()(_bindings.size());

            for (const VkDescriptorSetLayoutBinding& b : _bindings)
            {
                //pack the binding data into a single int64. Not fully correct but it's ok
                size_t binding_hash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;

                //shuffle the packed binding data and xor it with the main hash
                result ^= hash<size_t>()(binding_hash);
            }

            return result;
        }
    } // slag
} // vulkan