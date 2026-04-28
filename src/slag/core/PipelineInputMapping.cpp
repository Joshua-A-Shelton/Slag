#include "PipelineInputMapping.h"

#include <stdexcept>

#include "GraphicsCard.h"
#include "slag/exceptions/PipelineInputMappingSizeExceeded.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    DescriptorTableInput::DescriptorTableInput(DescriptorRange* ranges, uint32_t rangeCount)
    {
        _ranges.resize(rangeCount);
        memcpy(_ranges.data(),ranges,rangeCount*sizeof(DescriptorRange));
    }

    DescriptorTableInput::DescriptorTableInput(const DescriptorTableInput& from)
    {
        copy(from);
    }

    DescriptorTableInput& DescriptorTableInput::operator=(const DescriptorTableInput& from)
    {
        copy(from);
        return *this;
    }

    DescriptorTableInput::DescriptorTableInput(DescriptorTableInput&& from) noexcept
    {
        move(from);
    }

    DescriptorTableInput& DescriptorTableInput::operator=(DescriptorTableInput&& from) noexcept
    {
        move(from);
        return *this;
    }

    uint32_t DescriptorTableInput::rangeCount()const
    {
        return _ranges.size();
    }

    const DescriptorRange& DescriptorTableInput::descriptorRange(uint32_t index)const
    {
        SLAG_ASSERT(index < rangeCount());
        return _ranges[index];
    }

    void DescriptorTableInput::copy(const DescriptorTableInput& from)
    {
        _ranges = from._ranges;
    }

    void DescriptorTableInput::move(DescriptorTableInput& from)
    {
        _ranges.swap(from._ranges);
    }

    PipelineInput::PipelineInput(uint32_t location, const ConstantRangeInput& input)
    {
        _location = location;
        _binding = input;
        _type = PipelineInputType::CONSTANT_RANGE;
    }

    PipelineInput::PipelineInput(uint32_t location, const DescriptorInput& input)
    {
        _location = location;
        _binding = input;
        _type = PipelineInputType::DESCRIPTOR;
    }

    PipelineInput::PipelineInput(uint32_t location, DescriptorTableInput&& input)
    {
        _location = location;
        _binding = std::move(input);
        _type = PipelineInputType::DESCRIPTOR_TABLE;
    }

    PipelineInput::PipelineInput(const PipelineInput& from)
    {
        copy(from);
    }

    PipelineInput& PipelineInput::operator=(const PipelineInput& from)
    {
        copy(from);
        return *this;
    }

    PipelineInput::PipelineInput(PipelineInput&& from) noexcept
    {
        move(from);
    }

    PipelineInput& PipelineInput::operator=(PipelineInput&& from) noexcept
    {
        move(from);
        return *this;
    }

    PipelineInputType PipelineInput::type() const
    {
        return _type;
    }

    uint32_t PipelineInput::location() const
    {
        return _location;
    }

    const ConstantRangeInput* PipelineInput::constantRange() const
    {
        if (auto* ptr = std::get_if<ConstantRangeInput>(&_binding))
        {
            return ptr;
        }
        return nullptr;
    }

    const DescriptorInput* PipelineInput::descriptor() const
    {
        if (auto* ptr = std::get_if<DescriptorInput>(&_binding))
        {
            return ptr;
        }
        return nullptr;
    }

    const DescriptorTableInput* PipelineInput::descriptorTable() const
    {
        if (auto* ptr = std::get_if<DescriptorTableInput>(&_binding))
        {
            return ptr;
        }
        return nullptr;
    }

    void PipelineInput::copy(const PipelineInput& from)
    {
        _binding = from._binding;
        _type = from._type;
        _location = from._location;
    }

    void PipelineInput::move(PipelineInput& from)
    {
        _binding.swap(from._binding);
        _type = from._type;
        _location = from._location;
    }

    PipelineInputMapping::PipelineInputMapping(const PipelineInput& from)
    {
        copy(from);
    }

    PipelineInputMapping& PipelineInputMapping::operator=(const PipelineInputMapping& from)
    {
        copy(from);
        return *this;
    }

    PipelineInputMapping::PipelineInputMapping(PipelineInputMapping&& from) noexcept
    {
        move(from);
    }

    PipelineInputMapping& PipelineInputMapping::operator=(PipelineInputMapping&& from) noexcept
    {
        move(from);
        return *this;
    }

    uint32_t PipelineInputMapping::inputCount() const
    {
        return _mappings.size();
    }

    const PipelineInput& PipelineInputMapping::input(uint32_t index)
    {
        return _mappings[index];
    }

    void PipelineInputMapping::copy(const PipelineInputMapping& from)
    {
        _mappings = from._mappings;
    }

    void PipelineInputMapping::move(PipelineInputMapping& from)
    {
        _mappings.swap(from._mappings);
    }

    PipelineInputMapping::PipelineInputMapping(std::vector<PipelineInput>&& mappings)
    {
        _mappings = std::move(mappings);
    }

    PipelineInputMappingBuilder::PipelineInputMappingBuilder(GraphicsCard* graphicsCard)
    {
        SLAG_ASSERT(graphicsCard != nullptr);
        _graphicsCard = graphicsCard;
    }

    PipelineInputMappingBuilder& PipelineInputMappingBuilder::addConstantRange(uint32_t bindGroupIndex,uint32_t binding, uint32_t rangeSize)
    {
        if (_inputSize + rangeSize > 512)
        {
            throw PipelineInputMappingSizeExceeded("Pipeline Mapping exceeds 512 bytes");
        }
        _mappings.emplace_back(_inputSize,ConstantRangeInput{.bindGroupIndex = bindGroupIndex,.binding = binding,.rangeSize = rangeSize});
        _inputSize += rangeSize;
        return *this;
    }

    PipelineInputMappingBuilder& PipelineInputMappingBuilder::addDescriptor(uint32_t bindGroupIndex, uint32_t binding,
        DescriptorType descriptor)
    {
        if (_inputSize + 4 > 512)
        {
            throw PipelineInputMappingSizeExceeded("Pipeline Mapping exceeds 512 bytes");
        }
        _mappings.emplace_back(_inputSize,DescriptorInput(bindGroupIndex,binding,descriptor));
        _inputSize += 4;
        return *this;
    }

    PipelineInputMappingBuilder& PipelineInputMappingBuilder::addDescriptorTable(DescriptorRange* ranges, uint32_t rangeCount)
    {
        if (_inputSize + (rangeCount * 4) > 512)
        {
            throw PipelineInputMappingSizeExceeded("Pipeline Mapping exceeds 512 bytes");
        }
        _mappings.emplace_back(_inputSize,DescriptorTableInput(ranges, rangeCount));
        _inputSize += (4 * rangeCount);
        return *this;
    }

    PipelineInputMapping PipelineInputMappingBuilder::build()
    {
        PipelineInputMapping mapping(std::move(_mappings));
        _mappings = std::vector<PipelineInput>();
        return mapping;
    }
} // slag