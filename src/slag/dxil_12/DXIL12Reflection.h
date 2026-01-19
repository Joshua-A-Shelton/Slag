#ifndef SLAG_DXIL12REFLECTION_H
#define SLAG_DXIL12REFLECTION_H
#include <unordered_map>
#include <slag/Slag.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <d3d12shader.h>

namespace slag
{
    namespace dxil
    {
        struct DXILDescriptorGroupReflectionData
        {
            uint32_t groupIndex = UINT32_MAX;
            std::vector<Descriptor> descriptors;
            std::vector<uint32_t> descriptorOffsets;
        };

        struct DXILReflectionData
        {
            Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
            std::vector<D3D12_INPUT_ELEMENT_DESC> vertexInputElements;
            std::vector<DXILDescriptorGroupReflectionData> groups;
            std::unordered_map<uint32_t, std::unordered_map<uint32_t,BufferLayout>> bufferLayouts;
            std::unordered_map<uint32_t,std::unordered_map<uint32_t,TexelBufferDescription>> texelBufferDescriptions;
            uint32_t entryPointXDim=0;
            uint32_t entryPointYDim=0;
            uint32_t entryPointZDim=0;
        };

        DXILReflectionData getReflectionData(ShaderCode** shaders, size_t shaderCount, DescriptorIdentity(*identify)(const DescriptorIdentityParameters&,void*), void* identifyData);
    }
} // slag

#endif //SLAG_DXIL12REFLECTION_H
