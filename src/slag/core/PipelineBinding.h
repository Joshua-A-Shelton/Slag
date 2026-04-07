#ifndef SLAG_PIPELINEBINDING_H
#define SLAG_PIPELINEBINDING_H
#include <map>

#include "Descriptors.h"
namespace slag
{
    ///Binding details for input into a shader pipeline
    class DescriptorTableEntry
    {
    public:
        ///Name of descriptor at this binding
        [[nodiscard]] const std::string& name()const;
        ///The type of descriptor at this binding
        [[nodiscard]] DescriptorType descriptorType()const;
        ///The offset in bytes from the beginning of the binding group
        [[nodiscard]] uint64_t offset()const;
        ///Sampler details
        [[nodiscard]] const SamplerDescription* samplerDetails()const;
        ///Texture details
        [[nodiscard]] const TextureDescription* textureDetails()const;
        ///Buffer details
        [[nodiscard]] const BufferDescription* bufferDetails()const;
        ///Texel buffer details
        [[nodiscard]] const TexelBufferDescription* texelBufferDetails()const;
        ///Acceleration buffer details
        [[nodiscard]] const AccelerationStructureDescription* accelerationStructureDetails()const;
        friend class PipelineBindingBuilder;
    private:
        DescriptorTableEntry(const std::string& name, uint64_t offset, DescriptorType type, const SamplerDescription& samplerDescription);
        DescriptorTableEntry(const std::string& name, uint64_t offset, DescriptorType type, const TextureDescription& textureDescription);
        DescriptorTableEntry(const std::string& name, uint64_t offset, DescriptorType type, const BufferDescription& bufferDescription);
        DescriptorTableEntry(const std::string& name, uint64_t offset, DescriptorType type, const TexelBufferDescription& texelBufferDescription);
        DescriptorTableEntry(const std::string& name, uint64_t offset, DescriptorType type, const AccelerationStructureDescription& accelerationStructureDescription);
        std::string _name;
        DescriptorType _descriptorType = DescriptorType::UNKNOWN;
        uint64_t _offset = 0;
        std::variant<SamplerDescription,TextureDescription,BufferDescription,TexelBufferDescription,AccelerationStructureDescription> _details;
    };

    ///A group a descriptors that get bound at the same time
    class DescriptorTable
    {
    public:
        ///The index this table is bound to
        [[nodiscard]] uint32_t binding() const;
        ///Number of descriptors in this table
        [[nodiscard]] uint32_t descriptorCount() const;
        ///Retrieve a descriptor at an index
        [[nodiscard]] const DescriptorTableEntry& descriptor(uint32_t index)const;
        ///total size in bytes of this table
        uint64_t size() const;
        friend class PipelineBindings;
    private:
        DescriptorTable(uint32_t binding, const std::vector<DescriptorTableEntry>& descriptorBindings, uint64_t tableSize);
        std::vector<DescriptorTableEntry> _descriptorBindings;
        uint64_t _tableSize=0;
        uint32_t _binding=0;

    };
    struct BindingStub
    {
        uint64_t currentSize = 0;
        std::vector<DescriptorTableEntry> tableEntries;
    };
    ///Describes how data is passed into a shader pipeline
    class PipelineBindings
    {
    public:
        ///Number of DescriptorTables in this binding
        [[nodiscard]] uint32_t descriptorTableCount() const;
        ///Retrieve a descriptor table at a given index
        [[nodiscard]] const DescriptorTable& descriptorTable(uint32_t index)const;
        ///Layout of push descriptor buffer, or null if no push descriptor is defined for the bindings
        BufferLayout* pushDescriptorLayout();
        friend class PipelineBindingBuilder;
    private:
        PipelineBindings(const std::map<uint32_t, BindingStub>& bindingMap,const BufferLayout& pushDescriptorLayout, bool hasPushDescriptor);
        std::vector<DescriptorTable> _descriptorTables;
        BufferLayout _pushDescriptorLayout{std::vector<StructMember>()};
        bool _hasPush = false;
    };
    ///Object to build PipelineBindings
    class PipelineBindingBuilder
    {
    public:
        /**
         * Create a new PipelineBindingBuilder
         * @param graphicsCard Graphics card that dictates how the end descriptor tables are layed out. Each Graphics card may produce it's own table layouts
         */
        PipelineBindingBuilder(GraphicsCard* graphicsCard);
        /**
         * Add a new texture sampler to a table
         * @param tableBinding Binding index of the table this sampler belongs to
         * @param name Name of the sampler in the descriptor table
         * @return
         */
        PipelineBindingBuilder& addSampler(uint32_t tableBinding, const std::string& name);
        /**
         * Add a new sampled texture to a table
         * @param tableBinding Binding index of the table this texture belongs to
         * @param name Name of the texture in the descriptor table
         * @param description Description of the texture
         * @return
         */
        PipelineBindingBuilder& addSampledTexture(uint32_t tableBinding, const std::string& name, TextureDescription description);
        /**
         * Add a new unordered access texture to a table
         * @param tableBinding Binding index of the table this texture belongs to
         * @param name Name of the texture in the descriptor table
         * @param description Description of the texture
         * @return
         */
        PipelineBindingBuilder& addUnorderedAccessTexture(uint32_t tableBinding, const std::string& name, TextureDescription description);
        /**
         * Add a new uniform buffer to a table
         * @param tableBinding Binding index of the table this buffer belongs to
         * @param name Name of the buffer in the descriptor table
         * @param layout The layout of the buffer
         * @return
         */
        PipelineBindingBuilder& addUniformBuffer(uint32_t tableBinding, const std::string& name, const BufferLayout& layout);
        /**
         * Add a new unordered access buffer to a table
         * @param tableBinding Binding index of the table this buffer belongs to
         * @param name Name of the buffer in the descriptor table
         * @return
         */
        PipelineBindingBuilder& addUnorderedAccessBuffer(uint32_t tableBinding, const std::string& name);
        /**
         * Add a new uniform texel buffer to a table
         * @param tableBinding Binding index of the table this buffer belongs to
         * @param name Name of the buffer in the descriptor table
         * @param format Texel format of the buffer
         * @return
         */
        PipelineBindingBuilder& addUniformTexelBuffer(uint32_t tableBinding, const std::string& name, PixelFormat format);
        /**
         * Add a new unordered access texel buffer to a table
         * @param tableBinding Binding index of the table this buffer belongs to
         * @param name Name of the buffer in the descriptor table
         * @param format Texel format of the buffer
         * @return
         */
        PipelineBindingBuilder& addUnorderedTexelBuffer(uint32_t tableBinding, const std::string& name, PixelFormat format);
        /**
         * Add a new acceleration structure to a table
         * @param tableBinding Binding index of the table this structure belongs to
         * @param name Name of the structure in the descriptor table
         * @param layout layout of the acceleration structure
         * @return
         */
        PipelineBindingBuilder& addAccelerationStructure(uint32_t tableBinding, const BufferLayout& layout, const std::string& name);
        /**
         * Add push constant layout to bindings
         * @param layout The layout of the push constant buffer
         * @return
         */
        PipelineBindingBuilder& addPushConstants(const BufferLayout& layout);
        /**
         * Create PipelineBindings
         * @param clear Clear the builder object
         * @return
         */
        PipelineBindings build(bool clear = false);
        ///Clear the builder object to start from fresh
        PipelineBindingBuilder& clear();
    private:
        bool _hasPush = false;
        std::map<uint32_t, BindingStub> _bindings;
        BufferLayout _pushConstantLayout{std::vector<StructMember>()};
        GraphicsCard* _card = nullptr;
    };
} // slag

#endif //SLAG_PIPELINEBINDING_H