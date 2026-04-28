#ifndef SLAG_SHADERCODE_H
#define SLAG_SHADERCODE_H
#include <cstdint>
#include <vector>

#include "Descriptors.h"

namespace slag
{
    enum class ShaderLanguage
    {
        SPIRV,
        DXIL,
        CUSTOM
    };
    ///Represents what kind of execution the shader performs. On many graphics cards, certain stages have bespoke hardware, and needs to be specified
    enum class ShaderType
    {
        ///Processes vertex attributes
        VERTEX,
        ///Processes geometry primitives (point/line/triangle), and can emit new geometry (Not recommended, use vertex or mesh shaders instead)
        GEOMETRY,
        ///Outputs values to color and depth targets
        FRAGMENT,
        ///General purpose computation
        COMPUTE,
        ///Emits the rays that are used in ray tracing
        RAY_GENERATION,
        ///Runs before CLOSEST_HIT, and can discard hits
        ANY_HIT,
        ///Processes Rays that hit geometry
        CLOSEST_HIT,
        ///Processes Rays that don't hit geometry
        MISS,
        ///Computes intersection details for geometry
        INTERSECTION,
        ///A sub shader that is called from another shader
        CALLABLE,
        ///Generates Mesh data from arbitrary output
        MESH,
        ///Prepares work for the MESH shaders
        TASK
    };
    ///Collection of descriptors that get bound together
    class BindGroup
    {
    public:
        BindGroup(uint32_t groupIndex, std::vector<DescriptorMeta>&& descriptorInfo);
        ///The identifier (DX12 register space/ Vulkan descriptor set) of the bind group
        [[nodiscard]] uint32_t groupIndex()const;
        ///Total number of descriptor data this bind group contains
        [[nodiscard]] uint32_t descriptorInfoCount()const;
        /**
         * Gets the descriptor data for a descriptor in this group
         * @param index Index of descriptor data to retrieve
         * @return
         */
        [[nodiscard]] const DescriptorMeta& descriptorInfo(uint32_t index)const;
    private:
        uint32_t _groupIndex=0;
        std::vector<DescriptorMeta> _descriptorInfo;
    };

    ///Variable that serves as an input or output of a shader
    class ShaderInterfaceVariable
    {
    public:
        ShaderInterfaceVariable(const std::string& name, GraphicsType type, uint32_t location);
        [[nodiscard]] const std::string& name()const;
        [[nodiscard]] GraphicsType type()const;
        [[nodiscard]] uint32_t location()const;
    private:
        std::string _name;
        GraphicsType _type;
        uint32_t _location;

    };
    ///Contains all the metadata about a shader
    class ShaderMetaData
    {
    public:
        ShaderMetaData()=default;
        ShaderMetaData(ShaderType type, std::vector<BindGroup>&& bindGroups, std::vector<ShaderInterfaceVariable>&& inputVariables, std::vector<ShaderInterfaceVariable>&& outputVariables);
        ///What kind of shader this is
        [[nodiscard]] ShaderType type() const;
        ///Number of bind groups
        [[nodiscard]] uint32_t bindGroupCount() const;
        /**
         * Get a bind group
         * @param index Index of bind group. (NOT the bindIndex of the group itself, just the entry into the array)
         * @return
         */
        [[nodiscard]] const BindGroup& bindGroup(uint32_t index)const;
        ///Number of input variables into the shader
        [[nodiscard]] uint32_t inputVariableCount()const;
        /**
         * Get an input variable
         * @param index Index of input variable
         * @return
         */
        [[nodiscard]] const ShaderInterfaceVariable& inputVariable(uint32_t index)const;
        ///Number of output variables from the shader
        [[nodiscard]] uint32_t outputVariableCount ()const;
        /**
         * Get an output variable
         * @param index Index of output variable
         * @return
         */
        [[nodiscard]] const ShaderInterfaceVariable& outputVariable(uint32_t index)const;
    private:
        ShaderType _type = ShaderType::VERTEX;
        std::vector<BindGroup> _bindGroups;
        std::vector<ShaderInterfaceVariable> _inputVariables;
        std::vector<ShaderInterfaceVariable> _outputVariables;
    };
    ///All the data for a shader stage that is ready to be fed into a shader pipeline
    class ShaderModule
    {
    public:
        virtual ~ShaderModule()=default;
        ///Language this shader module is
        [[nodiscard]] virtual ShaderLanguage shaderLanguage() const = 0;
        ///Metadata details about the shader itself
        [[nodiscard]] virtual const ShaderMetaData& metaData() = 0;
        ///The graphics card this module is on
        [[nodiscard]] virtual GraphicsCard* graphicsCard()const = 0;
    };
} // slag

#endif //SLAG_SHADERCODE_H
