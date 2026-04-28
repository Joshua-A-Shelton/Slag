#ifndef SLAG_PIPELINEBINDING_H
#define SLAG_PIPELINEBINDING_H
#include "Descriptors.h"
namespace slag
{
   ///What kind of input a PipelineInput is
   enum class PipelineInputType
   {
      CONSTANT_RANGE,
      DESCRIPTOR,
      DESCRIPTOR_TABLE
   };
   ///Input is flat primitive data, such as integers or matrices
   struct ConstantRangeInput
   {
      ///What register space (DX12)/ descriptor set (Vulkan) this input corresponds to in the shaders that makes up the pipeline
      uint32_t bindGroupIndex;
      ///The element in the bind group index the range corresponds to
      uint32_t binding;
      ///Size in bytes of the constant range
      uint32_t rangeSize;
   };

   struct DescriptorInput
   {
      uint32_t bindGroupIndex;
      uint32_t binding;
      DescriptorType descriptor;
   };

   enum class DescriptorRangeType
   {
      UNIFORM_BUFFER,
      READONLY_RESOURCE,
      UNORDERED_ACCESS_RESOURCE,
      SAMPLER
   };
   struct DescriptorRange
   {
      DescriptorRangeType type;
      uint32_t bindGroupIndex;
      uint32_t firstBinding;
      uint32_t bindingCount;
      uint32_t offsetInDescriptorsFromTableStart;
   };

   class DescriptorTableInput
   {
   public:
      DescriptorTableInput(DescriptorRange* ranges, uint32_t rangeCount);
      DescriptorTableInput(const DescriptorTableInput& from);
      DescriptorTableInput& operator=(const DescriptorTableInput& from);
      DescriptorTableInput(DescriptorTableInput&& from) noexcept;
      DescriptorTableInput& operator=(DescriptorTableInput&& from) noexcept;
      [[nodiscard]] uint32_t rangeCount()const;
      [[nodiscard]] const DescriptorRange& descriptorRange(uint32_t index)const;
   private:
      void copy(const DescriptorTableInput& from);
      void move(DescriptorTableInput& from);
      std::vector<DescriptorRange> _ranges;
   };

   class PipelineInput
   {
   public:
      PipelineInput(uint32_t location, const ConstantRangeInput& input);
      PipelineInput(uint32_t location, const DescriptorInput& input);
      PipelineInput(uint32_t location, DescriptorTableInput&& input);
      PipelineInput(const PipelineInput& from);
      PipelineInput& operator=(const PipelineInput& from);
      PipelineInput(PipelineInput&& from) noexcept;
      PipelineInput& operator=(PipelineInput&& from) noexcept;
      [[nodiscard]] PipelineInputType type() const;
      [[nodiscard]] uint32_t location() const;
      [[nodiscard]] const ConstantRangeInput* constantRange()const;
      [[nodiscard]] const DescriptorInput* descriptor()const;
      [[nodiscard]] const DescriptorTableInput* descriptorTable()const;
   private:
      void copy(const PipelineInput& from);
      void move(PipelineInput& from);
      std::variant<ConstantRangeInput,DescriptorInput,DescriptorTableInput> _binding;
      PipelineInputType _type = PipelineInputType::CONSTANT_RANGE;
      uint32_t _location = 0;

   };

   class PipelineInputMapping
   {
   public:
      PipelineInputMapping(std::vector<PipelineInput>&& mappings);
      PipelineInputMapping(const PipelineInput& from);
      PipelineInputMapping& operator=(const PipelineInputMapping& from);
      PipelineInputMapping(PipelineInputMapping&& from) noexcept;
      PipelineInputMapping& operator=(PipelineInputMapping&& from) noexcept;
      [[nodiscard]] uint32_t inputCount() const;
      const PipelineInput& input(uint32_t index);
   private:
      void copy(const PipelineInputMapping& from);
      void move(PipelineInputMapping& from);

      std::vector<PipelineInput> _mappings;
   };

   class PipelineInputMappingBuilder
   {
   public:
      PipelineInputMappingBuilder(GraphicsCard* graphicsCard);
      PipelineInputMappingBuilder& addConstantRange(uint32_t bindGroupIndex, uint32_t binding, uint32_t rangeSize);
      PipelineInputMappingBuilder& addDescriptor(uint32_t bindGroupIndex, uint32_t binding, DescriptorType descriptor);
      PipelineInputMappingBuilder& addDescriptorTable(DescriptorRange* ranges, uint32_t rangeCount);
      PipelineInputMapping build();
   private:
      GraphicsCard* _graphicsCard = nullptr;
      uint32_t _inputSize = 0;
      std::vector<PipelineInput> _mappings;
   };
} // slag

#endif //SLAG_PIPELINEBINDING_H