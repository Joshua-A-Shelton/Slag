#include "VulkanDescriptorGroup.h"

#include <algorithm>

#include "VulkanDescriptorGroupCache.h"
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

            _bindings.resize(descriptorCount);
            _descriptorNames.resize(descriptorCount);
            std::vector<Descriptor::Shape> descriptorShapes(descriptorCount);
            for(uint32_t i=0; i< descriptorCount; i++)
            {
                auto desc = descriptors[i];
                descriptorShapes[i] = desc.shape();
                SLAG_ASSERT(std::find(_descriptorNames.begin(), _descriptorNames.end(), desc.name()) == _descriptorNames.end() && "Duplicate descriptor name");
                _descriptorNames[i] = desc.name();
                _descriptorIndexMap.insert(std::make_pair(desc.name(), VulkanDescriptorIndex{.binding = i}));
                _bindings[i] = std::move(desc);
            }
            //names should be in alphabetical order
            std::sort(_descriptorNames.begin(), _descriptorNames.end());
            _groupShape._descriptorShapes = std::move(descriptorShapes);
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

        Descriptor& VulkanDescriptorGroup::descriptorAtBinding(uint32_t binding)
        {
            return _bindings[binding];
        }

        void VulkanDescriptorGroup::copy(const VulkanDescriptorGroup& from)
        {
            _bindings = from._bindings;
            //do this instead of direct copy to increase the internal reference count
            _layout = VulkanDescriptorGroupCache::getLayout(*this);
             _descriptorNames = from._descriptorNames;
            _descriptorIndexMap = from._descriptorIndexMap;
            _groupShape = from._groupShape;
        }

        void VulkanDescriptorGroup::move(VulkanDescriptorGroup& from)
        {
            std::swap(_layout,from._layout);
            _bindings.swap(from._bindings);
            _descriptorNames.swap(from._descriptorNames);
            _descriptorIndexMap.swap(from._descriptorIndexMap);
            _groupShape = from._groupShape;
        }

        uint32_t VulkanDescriptorGroup::descriptorCount()
        {
            return _bindings.size();
        }

        std::string VulkanDescriptorGroup::descriptorName(uint32_t index)
        {
             return _descriptorNames[index];
        }

        DescriptorIndex* VulkanDescriptorGroup::indexOf(const std::string& descriptorName)
        {
             auto index = _descriptorIndexMap.find(descriptorName);
             if (index != _descriptorIndexMap.end())
             {
                 return &index->second;
             }
             return nullptr;
        }

        Descriptor* VulkanDescriptorGroup::descriptor(DescriptorIndex* index)
        {
             auto vIndex = static_cast<VulkanDescriptorIndex*>(index);
             return &_bindings[vIndex->binding];
        }

        Descriptor* VulkanDescriptorGroup::descriptor(const std::string& descriptorName)
        {
             auto index = _descriptorIndexMap.find(descriptorName);
             if (index != _descriptorIndexMap.end())
             {
                 return &_bindings[index->second.binding];
             }
             return nullptr;
        }

        bool VulkanDescriptorGroup::compatible(DescriptorGroup* with)
        {
             auto against = static_cast<VulkanDescriptorGroup*>(with);
             for (int i=0; i< _bindings.size() && i<against->_bindings.size(); i++)
             {
                 auto& me = _bindings[i];
                 auto& againstme = against->_bindings[i];
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
