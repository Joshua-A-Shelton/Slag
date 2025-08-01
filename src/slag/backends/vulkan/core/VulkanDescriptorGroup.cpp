#include "VulkanDescriptorGroup.h"

#include "VulkanDescriptorGroupCache.h"

namespace slag
{
    namespace vulkan
    {
         bool compareDescriptorIndex(const Descriptor& a, const Descriptor& b)
        {
            return a.shape().binding < b.shape().binding;
        }
        VulkanDescriptorGroup::VulkanDescriptorGroup(Descriptor* descriptors, size_t descriptorCount)
        {
            _bindings.resize(descriptorCount);
            size_t lastIndex = 0;
            bool sorted = true;
            for(size_t i=0; i< descriptorCount; i++)
            {
                auto desc = descriptors[i];
                if(desc.shape().binding >= lastIndex)
                {
                    lastIndex = desc.shape().binding;
                }
                else
                {
                    sorted = false;
                }
                _bindings[i] = std::move(desc);
            }
            if(!sorted)
            {
                std::sort(_bindings.begin(),_bindings.end(),compareDescriptorIndex);
            }
            _layout = VulkanDescriptorGroupCache::getLayout(*this);
        }

        VulkanDescriptorGroup::~VulkanDescriptorGroup()
        {
            if(_layout)
            {
                VulkanDescriptorGroupCache::removeInstance(*this);
            }
        }

        VulkanDescriptorGroup::VulkanDescriptorGroup(const VulkanDescriptorGroup& from)
        {
            copy(from);
        }

        VulkanDescriptorGroup& VulkanDescriptorGroup::operator=(const VulkanDescriptorGroup& from)
        {
            copy(from);
            return *this;
        }

        VulkanDescriptorGroup::VulkanDescriptorGroup(VulkanDescriptorGroup&& from)
        {
            move(from);
        }

        VulkanDescriptorGroup& VulkanDescriptorGroup::operator=(VulkanDescriptorGroup&& from)
        {
            move(from);
            return *this;
        }

        void VulkanDescriptorGroup::copy(const VulkanDescriptorGroup& from)
        {
            _bindings = from._bindings;
            //do this instead of direct copy to increase the internal reference count
            _layout = VulkanDescriptorGroupCache::getLayout(*this);
        }

        void VulkanDescriptorGroup::move(VulkanDescriptorGroup& from)
        {
            std::swap(_layout,from._layout);
            _bindings.swap(from._bindings);
        }

        uint32_t VulkanDescriptorGroup::descriptorCount()
        {
            return _bindings.size();
        }

        Descriptor& VulkanDescriptorGroup::operator[](size_t index)
        {
            return _bindings[index];
        }

        Descriptor& VulkanDescriptorGroup::descriptor(size_t index)
        {
            return _bindings.at(index);
        }

        VkDescriptorSetLayout VulkanDescriptorGroup::layout() const
        {
            return _layout;
        }
    } // vulkan
} // slag
