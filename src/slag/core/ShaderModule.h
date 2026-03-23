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
        BindGroup(uint32_t bindIndex, std::vector<DescriptorMeta>&& descriptorInfo);
        ///The index of the bind group
        [[nodiscard]] uint32_t bindIndex()const;
        ///Total number of descriptor data this bind group contains
        [[nodiscard]] uint32_t descriptorInfoCount()const;
        /**
         * Gets the descriptor data for a descriptor in this group
         * @param index Index of descriptor data to retrieve
         * @return
         */
        [[nodiscard]] const DescriptorMeta& descriptorInfo(uint32_t index)const;
    private:
        uint32_t _bindIndex=0;
        std::vector<DescriptorMeta> _descriptorInfo;
    };

    ///Input variable
    class InputVariable
    {
    public:
        InputVariable(const std::string& name, GraphicsType type, uint32_t location);
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
        ShaderMetaData(ShaderType type, std::vector<BindGroup>&& bindGroups, std::vector<InputVariable>&& inputVariables);
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
        [[nodiscard]] const uint32_t inputVariableCount()const;
        [[nodiscard]] const InputVariable& inputVariable(uint32_t index)const;
    private:
        ShaderType _type = ShaderType::VERTEX;
        std::vector<BindGroup> _bindGroups;
        std::vector<InputVariable> _inputVariables;
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
    };
} // slag

#endif //SLAG_SHADERCODE_H
