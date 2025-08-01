#ifndef SLAG_SPIRVREFLECTION_H
#define SLAG_SPIRVREFLECTION_H
#include <unordered_map>
#include <slag/Slag.h>
namespace slag
{
    namespace spirv
    {
        struct SPVDescriptorGroupReflectionData
        {
            uint32_t groupIndex;
            std::vector<Descriptor> descriptors;
        };

        struct SPVReflectionData
        {
            std::vector<SPVDescriptorGroupReflectionData> groups;
            std::unordered_map<uint32_t, std::unordered_map<uint32_t,UniformBufferDescriptorLayout>> bufferLayouts;
        };

        SPVReflectionData getReflectionData(ShaderCode** shaders, size_t shaderCount);
    } // spirv
} // slag

#endif //SLAG_SPIRVREFLECTION_H
