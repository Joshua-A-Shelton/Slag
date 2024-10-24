#include "DescriptorGroup.h"
#include "BackEnd/BackEndLib.h"
namespace slag
{
    bool DescriptorGroup::Shape::operator==(const DescriptorGroup::Shape& to)const
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

    bool DescriptorGroup::Shape::operator!=(const DescriptorGroup::Shape& to)const
    {
        return !(*this == to);
    }


    size_t DescriptorGroup::Shape::DescriptorGroupShapeHash::operator()(const DescriptorGroup::Shape& shape)const
    {
        using std::size_t;
        using std::hash;

        size_t result = hash<size_t>()(shape._descriptorShapes.size());

        for (const Descriptor::Shape& descShape : shape._descriptorShapes)
        {
            //pack the binding data into a single int64. Not fully correct but it's ok
            size_t binding_hash = descShape.binding | descShape.type << 8 | descShape.arrayDepth << 16 | std::bit_cast<int>(descShape.visibleStages) << 24;

            //shuffle the packed binding data and xor it with the main hash
            result ^= hash<size_t>()(binding_hash);
        }

        return result;
    }

    DescriptorGroup::Shape DescriptorGroup::shape()
    {
        Shape s;
        auto count = descriptorCount();
        for(int i=0; i<count; i++)
        {
            s._descriptorShapes.push_back((*this)[i].shape());
        }
        return s;
    }

    DescriptorGroup* DescriptorGroup::newDescriptorGroup(Descriptor* descriptors, size_t descriptorCount)
    {
        return lib::BackEndLib::get()->newDescriptorGroup(descriptors,descriptorCount);
    }
} // slag