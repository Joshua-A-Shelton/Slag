#ifndef SLAG_DXIL12REFLECTION_H
#define SLAG_DXIL12REFLECTION_H
#include <unordered_map>
#include <slag/Slag.h>
#include <wrl/client.h>
#include <d3d12.h>
namespace slag
{
    namespace dxil
    {
        struct IndexedDescriptor
        {
            Descriptor descriptor;
            uint32_t index;
        };
        struct DXILDescriptorGroupReflectionData
        {
            uint32_t groupIndex;
            std::vector<IndexedDescriptor> descriptors;
        };

        struct DXILReflectionData
        {
            Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
            std::vector<DXILDescriptorGroupReflectionData> groups;
            std::unordered_map<uint32_t, std::unordered_map<uint32_t,BufferLayout>> bufferLayouts;
            std::unordered_map<uint32_t,std::unordered_map<uint32_t,TexelBufferDescription>> texelBufferDescriptions;
            uint32_t entryPointXDim=0;
            uint32_t entryPointYDim=0;
            uint32_t entryPointZDim=0;
        };

        DXILReflectionData getReflectionData(ShaderCode** shaders, size_t shaderCount, std::string(*rename)(const std::string&,uint32_t descriptorGroupIndex,Descriptor::Type type, uint32_t platformBindingIndex,void*), void* renameData);
    }
} // slag

#endif //SLAG_DXIL12REFLECTION_H
