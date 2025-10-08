#include "DXIL12Reflection.h"
#include <dxcapi.h>
#include <d3d12shader.h>

#include <wrl/client.h>

#include "slag/backends/dx12/DX12Backend.h"

namespace slag
{
    namespace dxil
    {
        Descriptor::Type descriptorType(D3D_SHADER_INPUT_TYPE type, D3D_SRV_DIMENSION dimension)
        {
            switch (type)
            {
            case D3D_SIT_CBUFFER:
                return Descriptor::Type::UNIFORM_BUFFER;
            case D3D_SIT_TBUFFER:
                return Descriptor::Type::UNIFORM_TEXEL_BUFFER;
            case D3D_SIT_TEXTURE:
                if (dimension == D3D_SRV_DIMENSION_BUFFER )
                {
                    return Descriptor::Type::UNIFORM_TEXEL_BUFFER;
                }
                return Descriptor::Type::SAMPLED_TEXTURE;
            case D3D_SIT_SAMPLER:
                return Descriptor::Type::SAMPLER;
            case D3D_SIT_UAV_RWTYPED:
                if (dimension == D3D_SRV_DIMENSION_BUFFER )
                {
                    return Descriptor::Type::STORAGE_TEXEL_BUFFER;
                }
                return Descriptor::Type::STORAGE_TEXTURE;
            case D3D_SIT_STRUCTURED:
                return Descriptor::Type::STORAGE_BUFFER;
            case D3D_SIT_UAV_RWSTRUCTURED:
                break;
            case D3D_SIT_BYTEADDRESS:
                break;
            case D3D_SIT_UAV_RWBYTEADDRESS:
                break;
            case D3D_SIT_UAV_APPEND_STRUCTURED:
                break;
            case D3D_SIT_UAV_CONSUME_STRUCTURED:
                break;
            case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                break;
            }
            return Descriptor::Type::UNKNOWN;
        }

        Descriptor::Dimension descriptorDimension(D3D_SRV_DIMENSION dimension)
        {
            switch (dimension)
            {
            case D3D_SRV_DIMENSION_TEXTURE1D:
            case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
                return Descriptor::Dimension::ONE_DIMENSIONAL;
            case D3D_SRV_DIMENSION_TEXTURE2D:
            case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
            case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
            case D3D_SRV_DIMENSION_TEXTURE2DMS:
                return Descriptor::Dimension::TWO_DIMENSIONAL;
            case D3D_SRV_DIMENSION_TEXTURE3D:
                return Descriptor::Dimension::THREE_DIMENSIONAL;
            case D3D_SRV_DIMENSION_TEXTURECUBE:
            case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
                return Descriptor::Dimension::CUBE;
            default:
                return Descriptor::Dimension::ONE_DIMENSIONAL;
            }
            return Descriptor::Dimension::UNKNOWN;
        }


        DXILReflectionData getReflectionData(ShaderCode** shaders, size_t shaderCount,std::string(*rename)(const DescriptorRenameParameters&,void*), void* renameData)
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
                    auto type = descriptorType(bindDesc.Type,bindDesc.Dimension);
                    auto dimension = descriptorDimension(bindDesc.Dimension);
                    if (type == Descriptor::Type::UNKNOWN)
                    {
                        throw std::runtime_error(std::string("Unsupported descriptor type for variable \"")+bindDesc.Name+"\" in dxil shader code\"");
                    }
                    if (rename!=nullptr)
                    {
                        DescriptorRenameParameters renameParameters{};
                        renameParameters.language = ShaderCode::CodeLanguage::DXIL;
                        renameParameters.originalName = name;
                        renameParameters.descriptorGroupIndex = bindDesc.Space;
                        renameParameters.type = type;
                        renameParameters.dimension = dimension;
                        renameParameters.arrayDepth = bindDesc.BindCount;
                        renameParameters.platformSpecificBindingIndex = bindDesc.BindPoint;
                        name = rename(renameParameters,renameData);
                    }
                    //TODO: do something with data....
                }
            }

            return DXILReflectionData();
        }
    }
} // slag
