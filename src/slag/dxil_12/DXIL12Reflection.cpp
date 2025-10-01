#include "DXIL12Reflection.h"
#include <dxcapi.h>
#include <d3d12shader.h>

#include <wrl/client.h>

#include "slag/backends/dx12/DX12Backend.h"

namespace slag
{
    namespace dxil
    {
        DXILReflectionData getReflectionData(ShaderCode** shaders, size_t shaderCount,std::string(*rename)(const std::string&,uint32_t,Descriptor::Type, uint32_t,void*), void* renameData)
        {
            throw std::runtime_error("DXILReflectionData::getReflectionData not supported yet");
            //TODO: move this out of here, I shouldn't create and destroy this every time
            Microsoft::WRL::ComPtr<IDxcUtils> dxilUtils = nullptr;
            DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxilUtils));

            for (auto i=0; i < shaderCount; i++)
            {
                auto shader = shaders[i];

                DxcBuffer buffer
                {
                    .Ptr = shader->data(),
                    .Size = shader->dataSize(),
                    .Encoding = 0
                };
                Microsoft::WRL::ComPtr<ID3D12ShaderReflection> shaderReflection{};
                dxilUtils->CreateReflection(&buffer,IID_PPV_ARGS(&shaderReflection));

                D3D12_SHADER_DESC shaderDesc{};
                shaderReflection->GetDesc(&shaderDesc);

                for (auto boundResourceIndex=0; boundResourceIndex< shaderDesc.BoundResources; boundResourceIndex++)
                {
                    D3D12_SHADER_INPUT_BIND_DESC bindDesc{};
                    shaderReflection->GetResourceBindingDesc(boundResourceIndex,&bindDesc);
                    std::string name = bindDesc.Name;
                    auto type = dx12::DX12Backend::dx12DescriptorType(bindDesc.Type,bindDesc.Dimension);
                    if (type == Descriptor::Type::UNKNOWN)
                    {
                        throw std::runtime_error(std::string("Unsupported descriptor type for variable \"")+bindDesc.Name+"\" in dxil shader code\"");
                    }
                    if (rename!=nullptr)
                    {
                        name = rename(name,bindDesc.Space,type,bindDesc.BindPoint,renameData);
                    }
                    //TODO: do something with data....
                }
            }

            return DXILReflectionData();
        }
    }
} // slag
