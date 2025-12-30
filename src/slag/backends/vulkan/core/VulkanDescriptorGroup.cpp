#include "VulkanDescriptorGroup.h"

#include <algorithm>

#include "VulkanDescriptorGroupCache.h"
#include "VulkanGraphicsCard.h"
#include "slag/backends/vulkan/VulkanExtensions.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {


        bool VulkanDescriptorGroup::Shape::operator==(const Shape& to) const
        {
            if(_descriptorShapes.size() != to._descriptorShapes.size())
            {
                return false;
            }
            bool isMatch = true;
            for(size_t i=0; i< _descriptorShapes.size(); i++)
            {
                if(_descriptorShapes[i] != to._descriptorShapes[i])
                {
                    isMatch = false;
                    break;
                }
            }
            return isMatch;
        }

        bool VulkanDescriptorGroup::Shape::operator!=(const Shape& to) const
        {
            return !(*this == to);
        }

        size_t VulkanDescriptorGroup::Shape::DescriptorGroupShapeHash::operator()(const Shape& shape) const
        {
            using std::size_t;
            using std::hash;

            size_t result = hash<size_t>()(shape._descriptorShapes.size());

            auto binding=0;
            for (const Descriptor::Shape& descShape : shape._descriptorShapes)
            {
                //pack the binding data into a single int64. Not fully correct but it's ok
                size_t binding_hash = binding | std::bit_cast<uint16_t>(descShape.type) << 8 | descShape.arrayDepth << 16 | std::bit_cast<uint16_t>(descShape.visibleStages) << 24;

                //shuffle the packed binding data and xor it with the main hash
                result ^= hash<size_t>()(binding_hash);
                binding++;
            }

            return result;
        }

        VulkanDescriptorGroup::VulkanDescriptorGroup(Descriptor* descriptors, uint32_t descriptorCount)
        {

            _descriptors.resize(descriptorCount);
            _descriptorByteOffsets.resize(descriptorCount);
            std::vector<Descriptor::Shape> descriptorShapes(descriptorCount);
            for(uint32_t i=0; i< descriptorCount; i++)
            {
                auto desc = descriptors[i];
                descriptorShapes[i] = desc.shape();
                _descriptors[i] = std::move(desc);
            }
            _groupShape._descriptorShapes = std::move(descriptorShapes);
            _layout = VulkanDescriptorGroupCache::getLayout(*this);

            slag_vkGetDescriptorSetLayoutSizeEXT(VulkanGraphicsCard::selected()->device(),_layout,&_descriptorBufferSize);
            for (auto i = 0; i<descriptorCount; i++)
            {
                VkDeviceSize offset = 0;
                slag_vkGetDescriptorSetLayoutBindingOffsetEXT(VulkanGraphicsCard::selected()->device(),_layout,i,&offset);
                _descriptorByteOffsets[i] = offset;
            }
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
            _descriptors = from._descriptors;
            _descriptorByteOffsets = from._descriptorByteOffsets;
            //do this instead of direct copy to increase the internal reference count
            _layout = VulkanDescriptorGroupCache::getLayout(*this);
            _groupShape = from._groupShape;
            _descriptorBufferSize = from._descriptorBufferSize;
        }

        void VulkanDescriptorGroup::move(VulkanDescriptorGroup& from)
        {
            std::swap(_layout,from._layout);
            _descriptors.swap(from._descriptors);
            _descriptorByteOffsets.swap(from._descriptorByteOffsets);
            _groupShape = from._groupShape;
            _descriptorBufferSize = from._descriptorBufferSize;
        }

        uint32_t VulkanDescriptorGroup::descriptorCount()
        {
            return _descriptors.size();
        }

        const Descriptor& VulkanDescriptorGroup::descriptor(uint32_t index)
        {
            return _descriptors[index];
        }

        uint64_t VulkanDescriptorGroup::descriptorByteOffset(uint32_t index)
        {
            return _descriptorByteOffsets[index];
        }

        uint64_t VulkanDescriptorGroup::descriptorBufferSize()
        {
            return _descriptorBufferSize;
        }

        bool VulkanDescriptorGroup::compatible(DescriptorGroup* with)
        {
             auto against = static_cast<VulkanDescriptorGroup*>(with);
             for (int i=0; i< _descriptors.size() && i<against->_descriptors.size(); i++)
             {
                 auto& me = _descriptors[i];
                 auto& againstme = against->_descriptors[i];
                 if (me.shape() != againstme.shape())
                 {
                     return false;
                 }
             }
             return true;
        }


        VkDescriptorSetLayout VulkanDescriptorGroup::layout() const
        {
            return _layout;
        }

        VulkanDescriptorGroup::Shape& VulkanDescriptorGroup::groupShape()
        {
            return _groupShape;
        }
    } // vulkan
} // slag
