#include "PipelineBinding.h"

#include <stdexcept>

#include "GraphicsCard.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    const std::string& DescriptorTableEntry::name() const
    {
        return _name;
    }

    DescriptorType DescriptorTableEntry::descriptorType() const
    {
        return _descriptorType;
    }

    uint64_t DescriptorTableEntry::offset() const
    {
        return _offset;
    }

    const SamplerDescription* DescriptorTableEntry::samplerDetails() const
    {
        return std::get_if<SamplerDescription>(&_details);
    }

    const TextureDescription* DescriptorTableEntry::textureDetails() const
    {
        return std::get_if<TextureDescription>(&_details);
    }

    const BufferDescription* DescriptorTableEntry::bufferDetails() const
    {
        return std::get_if<BufferDescription>(&_details);
    }

    const TexelBufferDescription* DescriptorTableEntry::texelBufferDetails() const
    {
        return std::get_if<TexelBufferDescription>(&_details);
    }

    const AccelerationStructureDescription* DescriptorTableEntry::accelerationStructureDetails() const
    {
        return std::get_if<AccelerationStructureDescription>(&_details);
    }

    DescriptorTableEntry::DescriptorTableEntry(const std::string& name, uint64_t offset, DescriptorType type,
        const SamplerDescription& samplerDescription)
    {
        SLAG_ASSERT(type == DescriptorType::SAMPLER && "type must be DescriptorType::SAMPLER");
        _name = name;
        _offset = offset;
        _descriptorType = type;
        _details = samplerDescription;
    }

    DescriptorTableEntry::DescriptorTableEntry(const std::string& name, uint64_t offset, DescriptorType type,
        const TextureDescription& textureDescription)
    {
        SLAG_ASSERT((type == DescriptorType::SAMPLED_TEXTURE || type == DescriptorType::UNORDERED_ACCESS_TEXTURE) && "type must be DescriptorType::SAMPLED_TEXTURE or DescriptorType::UNORDERED_ACCESS_TEXTURE");
        _name = name;
        _offset = offset;
        _descriptorType = type;
        _details = textureDescription;
    }

    DescriptorTableEntry::DescriptorTableEntry(const std::string& name, uint64_t offset, DescriptorType type,
        const BufferDescription& bufferDescription)
    {
        SLAG_ASSERT((type == DescriptorType::UNIFORM_BUFFER || type == DescriptorType::UNORDERED_ACCESS_BUFFER) && "type must be DescriptorType::UNIFORM_BUFFER or DescriptorType::UNORDERED_ACCESS_BUFFER");
        _name = name;
        _offset = offset;
        _descriptorType = type;
        _details = bufferDescription;
    }

    DescriptorTableEntry::DescriptorTableEntry(const std::string& name, uint64_t offset, DescriptorType type, const TexelBufferDescription& texelBufferDescription)
    {
        SLAG_ASSERT((type == DescriptorType::UNIFORM_TEXEL_BUFFER || type == DescriptorType::UNORDERED_ACCESS_TEXEL_BUFFER) && "type must be DescriptorType::UNIFORM_TEXEL_BUFFER or DescriptorType::UNORDERED_ACCESS_TEXEL_BUFFER");
        _name = name;
        _offset = offset;
        _descriptorType = type;
        _details = texelBufferDescription;
    }

    DescriptorTableEntry::DescriptorTableEntry(const std::string& name, uint64_t offset, DescriptorType type,const AccelerationStructureDescription& accelerationStructureDescription)
    {
        SLAG_ASSERT((type == DescriptorType::ACCELERATION_STRUCTURE) && "type must be DescriptorType::ACCELERATION_STRUCTURE");
        _name = name;
        _offset = offset;
        _descriptorType = type;
        _details = accelerationStructureDescription;
    }

    uint32_t DescriptorTable::binding() const
    {
        return _binding;
    }

    uint32_t DescriptorTable::descriptorCount() const
    {
        return _descriptorBindings.size();
    }

    const DescriptorTableEntry& DescriptorTable::descriptor(uint32_t index) const
    {
        SLAG_ASSERT(index < _descriptorBindings.size());
        return _descriptorBindings[index];
    }

    uint64_t DescriptorTable::size() const
    {
        return _tableSize;
    }

    DescriptorTable::DescriptorTable(uint32_t binding, const std::vector<DescriptorTableEntry>& descriptorBindings, uint64_t tableSize)
    {
        _binding = binding;
        _descriptorBindings = descriptorBindings;
        _tableSize = tableSize;
    }

    uint32_t PipelineBindings::descriptorTableCount() const
    {
        return _descriptorTables.size();
    }

    const DescriptorTable& PipelineBindings::descriptorTable(uint32_t index) const
    {
        SLAG_ASSERT(index < _descriptorTables.size());
        return _descriptorTables[index];
    }

    BufferLayout* PipelineBindings::pushDescriptorLayout()
    {
        if (_hasPush)
        {
            return &_pushDescriptorLayout;
        }
        return nullptr;
    }

    PipelineBindings::PipelineBindings(const std::map<uint32_t, BindingStub>& bindingMap,
        const BufferLayout& pushDescriptorLayout, bool hasPushDescriptor)
    {
        _descriptorTables.reserve(bindingMap.size());
        for (const auto& entry : bindingMap)
        {
            DescriptorTable table(entry.first,entry.second.tableEntries,entry.second.currentSize);
            _descriptorTables.push_back(std::move(table));
        }
        _hasPush = hasPushDescriptor;
        if (hasPushDescriptor)
        {
            _pushDescriptorLayout = pushDescriptorLayout;
        }
    }

    PipelineBindingBuilder::PipelineBindingBuilder(GraphicsCard* graphicsCard)
    {
        SLAG_ASSERT(graphicsCard!=nullptr);
        _card = graphicsCard;
    }

    PipelineBindingBuilder& PipelineBindingBuilder::addSampler(uint32_t tableBinding, const std::string& name)
    {
        auto table = _bindings.find(tableBinding);
        auto& tableDetails = _card->descriptorTableDetails();
        if (table == _bindings.end())
        {
            table = _bindings.insert(std::make_pair(tableBinding,BindingStub{})).first;
        }
        auto& stub = table->second;
        auto nextAlign = (stub.currentSize + (uint64_t)tableDetails.samplerDescriptorAlignment -1) & ~((uint64_t)tableDetails.samplerDescriptorAlignment -1);
        stub.tableEntries.push_back(DescriptorTableEntry(name, nextAlign,DescriptorType::SAMPLER,SamplerDescription{}));
        nextAlign+=tableDetails.samplerDescriptorSize;
        stub.currentSize = nextAlign;
        return *this;
    }

    PipelineBindingBuilder& PipelineBindingBuilder::addSampledTexture(uint32_t tableBinding, const std::string& name, TextureDescription description)
    {
        auto table = _bindings.find(tableBinding);
        auto& tableDetails = _card->descriptorTableDetails();
        if (table == _bindings.end())
        {
            table = _bindings.insert(std::make_pair(tableBinding,BindingStub{})).first;
        }
        auto& stub = table->second;
        auto nextAlign = (stub.currentSize + (uint64_t)tableDetails.sampledTextureAlignment -1) & ~((uint64_t)tableDetails.sampledTextureAlignment -1);
        stub.tableEntries.push_back(DescriptorTableEntry(name, nextAlign,DescriptorType::SAMPLED_TEXTURE,description));
        nextAlign+=tableDetails.sampledTextureSize;
        stub.currentSize = nextAlign;
        return *this;
    }

    PipelineBindingBuilder& PipelineBindingBuilder::addUnorderedAccessTexture(uint32_t tableBinding,
        const std::string& name, TextureDescription description)
    {
        auto table = _bindings.find(tableBinding);
        auto& tableDetails = _card->descriptorTableDetails();
        if (table == _bindings.end())
        {
            table = _bindings.insert(std::make_pair(tableBinding,BindingStub{})).first;
        }
        auto& stub = table->second;
        auto nextAlign = (stub.currentSize + (uint64_t)tableDetails.unorderedAccessTextureAlignment -1) & ~((uint64_t)tableDetails.unorderedAccessTextureAlignment -1);
        stub.tableEntries.push_back(DescriptorTableEntry(name, nextAlign,DescriptorType::UNORDERED_ACCESS_TEXTURE,description));
        nextAlign+=tableDetails.unorderedAccessTextureSize;
        stub.currentSize = nextAlign;
        return *this;
    }

    PipelineBindingBuilder& PipelineBindingBuilder::addUniformBuffer(uint32_t tableBinding, const std::string& name,
        const BufferLayout& layout)
    {
        auto table = _bindings.find(tableBinding);
        auto& tableDetails = _card->descriptorTableDetails();
        if (table == _bindings.end())
        {
            table = _bindings.insert(std::make_pair(tableBinding,BindingStub{})).first;
        }
        auto& stub = table->second;
        auto nextAlign = (stub.currentSize + (uint64_t)tableDetails.uniformBufferAlignment -1) & ~((uint64_t)tableDetails.uniformBufferAlignment -1);
        stub.tableEntries.push_back(DescriptorTableEntry(name, nextAlign,DescriptorType::UNIFORM_BUFFER,BufferDescription{layout}));
        nextAlign+=tableDetails.uniformBufferSize;
        stub.currentSize = nextAlign;
        return *this;
    }

    PipelineBindingBuilder& PipelineBindingBuilder::addUnorderedAccessBuffer(uint32_t tableBinding,
        const std::string& name)
    {
        auto table = _bindings.find(tableBinding);
        auto& tableDetails = _card->descriptorTableDetails();
        if (table == _bindings.end())
        {
            table = _bindings.insert(std::make_pair(tableBinding,BindingStub{})).first;
        }
        auto& stub = table->second;
        auto nextAlign = (stub.currentSize + (uint64_t)tableDetails.unorderedAccessTexelBufferAlignment -1) & ~((uint64_t)tableDetails.unorderedAccessTexelBufferAlignment -1);
        stub.tableEntries.push_back(DescriptorTableEntry(name, nextAlign,DescriptorType::UNORDERED_ACCESS_BUFFER,BufferDescription{BufferLayout{std::vector<StructMember>()}}));
        nextAlign+=tableDetails.unorderedAccessBufferSize;
        stub.currentSize = nextAlign;
        return *this;
    }

    PipelineBindingBuilder& PipelineBindingBuilder::addUniformTexelBuffer(uint32_t tableBinding,
        const std::string& name, PixelFormat format)
    {
        auto table = _bindings.find(tableBinding);
        auto& tableDetails = _card->descriptorTableDetails();
        if (table == _bindings.end())
        {
            table = _bindings.insert(std::make_pair(tableBinding,BindingStub{})).first;
        }
        auto& stub = table->second;
        auto nextAlign = (stub.currentSize + (uint64_t)tableDetails.uniformTexelBufferAlignment -1) & ~((uint64_t)tableDetails.uniformTexelBufferAlignment -1);
        stub.tableEntries.push_back(DescriptorTableEntry(name, nextAlign,DescriptorType::UNIFORM_TEXEL_BUFFER,TexelBufferDescription{format}));
        nextAlign+=tableDetails.uniformTexelBufferSize;
        stub.currentSize = nextAlign;
        return *this;
    }

    PipelineBindingBuilder& PipelineBindingBuilder::addUnorderedTexelBuffer(uint32_t tableBinding,
        const std::string& name, PixelFormat format)
    {
        auto table = _bindings.find(tableBinding);
        auto& tableDetails = _card->descriptorTableDetails();
        if (table == _bindings.end())
        {
            table = _bindings.insert(std::make_pair(tableBinding,BindingStub{})).first;
        }
        auto& stub = table->second;
        auto nextAlign = (stub.currentSize + (uint64_t)tableDetails.unorderedAccessTexelBufferAlignment -1) & ~((uint64_t)tableDetails.unorderedAccessTexelBufferAlignment -1);
        stub.tableEntries.push_back(DescriptorTableEntry(name, nextAlign,DescriptorType::UNORDERED_ACCESS_TEXEL_BUFFER,TexelBufferDescription{format}));
        nextAlign+=tableDetails.unorderedAccessTexelBufferSize;
        stub.currentSize = nextAlign;
        return *this;
    }

    PipelineBindingBuilder& PipelineBindingBuilder::addAccelerationStructure(uint32_t tableBinding,
        const BufferLayout& layout, const std::string& name)
    {
        auto table = _bindings.find(tableBinding);
        auto& tableDetails = _card->descriptorTableDetails();
        if (table == _bindings.end())
        {
            table = _bindings.insert(std::make_pair(tableBinding,BindingStub{})).first;
        }
        auto& stub = table->second;
        auto nextAlign = (stub.currentSize + (uint64_t)tableDetails.accelerationStructureAlignment -1) & ~((uint64_t)tableDetails.accelerationStructureAlignment -1);
        stub.tableEntries.push_back(DescriptorTableEntry(name, nextAlign,DescriptorType::ACCELERATION_STRUCTURE,AccelerationStructureDescription{layout}));
        nextAlign+=tableDetails.accelerationStructureSize;
        stub.currentSize = nextAlign;
        return *this;
    }

    PipelineBindingBuilder& PipelineBindingBuilder::addPushConstants(const BufferLayout& layout)
    {
        if (_hasPush)
        {
            throw std::invalid_argument("Push constant is already set");
        }
        _pushConstantLayout = layout;
        _hasPush = true;
        return *this;
    }

    PipelineBindings PipelineBindingBuilder::build(bool clear)
    {
        PipelineBindings bindings(_bindings,_pushConstantLayout,_hasPush);
        if (clear)
        {
            this->clear();
        }
        return bindings;
    }

    PipelineBindingBuilder& PipelineBindingBuilder::clear()
    {
        _bindings.clear();
        _hasPush = false;
        _pushConstantLayout = {BufferLayout{std::vector<StructMember>()}};
        return *this;
    }
} // slag