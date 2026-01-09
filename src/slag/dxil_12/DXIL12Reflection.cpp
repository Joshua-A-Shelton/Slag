#include "DXIL12Reflection.h"
#include <dxcapi.h>
#include <d3d12shader.h>

#include <wrl/client.h>

#include <directx/d3dx12_root_signature.h>
#include "slag/backends/dx12/DX12Backend.h"
#include "slag/backends/dx12/core/DX12GraphicsCard.h"

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

        D3D12_DESCRIPTOR_RANGE_TYPE rangeType(Descriptor::Type type)
        {
            switch (type)
            {
                case Descriptor::Type::UNIFORM_BUFFER:
                    return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                case Descriptor::Type::STORAGE_BUFFER:
                    return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                case Descriptor::Type::SAMPLED_TEXTURE:
                    return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                case Descriptor::Type::STORAGE_TEXTURE:
                    return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                case Descriptor::Type::UNIFORM_TEXEL_BUFFER:
                    return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                case Descriptor::Type::STORAGE_TEXEL_BUFFER:
                    return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                case Descriptor::Type::SAMPLER:
                    return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                case Descriptor::Type::ACCELERATION_STRUCTURE:
                default:
                throw std::invalid_argument("Invalid descriptor type");
            }
        }

        struct DXPositionalDescriptor
        {
            Descriptor descriptor;
            uint32_t desiredOffset;
            D3D12_SHADER_INPUT_BIND_DESC reflectedBinding;
            uint32_t descriptorCountFromTableRoot=UINT32_MAX;
        };

        std::vector<DXPositionalDescriptor> DXILOrderDescriptors(
            const std::vector<DXPositionalDescriptor>& uniformBuffers,
            const std::vector<DXPositionalDescriptor>& uniformTexelBuffers,
            const std::vector<DXPositionalDescriptor>& storageBuffers,
            const std::vector<DXPositionalDescriptor>& storageTexelBuffers,
            const std::vector<DXPositionalDescriptor>& sampledTextures,
            const std::vector<DXPositionalDescriptor>& storageTextures,
            const std::vector<DXPositionalDescriptor>& samplers,
            const std::vector<DXPositionalDescriptor>& accelerationStructures)
        {
            std::vector<DXPositionalDescriptor> descriptors;
            descriptors.reserve(
                uniformBuffers.size() +
                uniformTexelBuffers.size() +
                storageBuffers.size() +
                storageTexelBuffers.size() +
                sampledTextures.size() +
                storageTextures.size() +
                samplers.size() +
                accelerationStructures.size()
                );
            for (uint32_t i = 0; i < uniformBuffers.size(); i++)
            {
                descriptors.emplace_back(uniformBuffers[i]);
            }
            for (uint32_t i = 0; i < uniformTexelBuffers.size(); i++)
            {
                descriptors.emplace_back(uniformTexelBuffers[i]);
            }
            for (uint32_t i = 0; i < storageBuffers.size(); i++)
            {
                descriptors.emplace_back(storageBuffers[i]);
            }
            for (uint32_t i = 0; i < storageTexelBuffers.size(); i++)
            {
                descriptors.emplace_back(storageTexelBuffers[i]);
            }
            for (uint32_t i = 0; i < sampledTextures.size(); i++)
            {
                descriptors.emplace_back(sampledTextures[i]);
            }
            for (uint32_t i = 0; i < storageTextures.size(); i++)
            {
                descriptors.emplace_back(storageTextures[i]);
            }
            for (uint32_t i = 0; i < samplers.size(); i++)
            {
                descriptors.emplace_back(samplers[i]);
            }
            for (uint32_t i = 0; i < accelerationStructures.size(); i++)
            {
                descriptors.emplace_back(accelerationStructures[i]);
            }
            for (uint32_t i = 0; i < descriptors.size(); i++)
            {
                descriptors[i].descriptorCountFromTableRoot = i;
            }
            return descriptors;
        }

        std::vector<DXPositionalDescriptor> DXILExtractUnorderedDescriptors(const std::unordered_map<std::string, DXPositionalDescriptor>& descriptors)
        {

            std::vector<DXPositionalDescriptor> uniformBuffers;
            std::vector<DXPositionalDescriptor> uniformTexelBuffers;
            std::vector<DXPositionalDescriptor> storageBuffers;
            std::vector<DXPositionalDescriptor> storageTexelBuffers;
            std::vector<DXPositionalDescriptor> sampledTextures;
            std::vector<DXPositionalDescriptor> storageTextures;
            std::vector<DXPositionalDescriptor> samplers;
            std::vector<DXPositionalDescriptor> accelerationStructures;
            for (auto& entry : descriptors)
            {
                switch (entry.second.descriptor.shape().type)
                {
                    case Descriptor::Type::UNKNOWN:
                        throw std::runtime_error("Unknown descriptor type");
                        break;
                case Descriptor::Type::SAMPLER:
                    samplers.push_back(entry.second);
                    break;
                case Descriptor::Type::SAMPLED_TEXTURE:
                    sampledTextures.push_back(entry.second);
                    break;
                case Descriptor::Type::STORAGE_TEXTURE:
                    storageTextures.push_back(entry.second);
                    break;
                case Descriptor::Type::UNIFORM_TEXEL_BUFFER:
                    uniformTexelBuffers.push_back(entry.second);
                    break;
                case Descriptor::Type::STORAGE_TEXEL_BUFFER:
                    storageTexelBuffers.push_back(entry.second);
                    break;
                case Descriptor::Type::UNIFORM_BUFFER:
                    uniformBuffers.push_back(entry.second);
                    break;
                case Descriptor::Type::STORAGE_BUFFER:
                    storageBuffers.push_back(entry.second);
                    break;
                case Descriptor::Type::ACCELERATION_STRUCTURE:
                    accelerationStructures.push_back(entry.second);
                    break;
                }
            }
            return DXILOrderDescriptors(uniformBuffers,uniformTexelBuffers,storageTexelBuffers,storageTexelBuffers,sampledTextures,storageTextures,samplers,accelerationStructures);
        }

        std::vector<DXPositionalDescriptor> DXILExtractOrderedDescriptors(const std::vector<DXPositionalDescriptor>& descriptors)
        {
            std::vector<DXPositionalDescriptor> orderedDescriptors(descriptors.size());
            for (auto& entry : descriptors)
            {
                auto& desc = entry;
                orderedDescriptors[desc.desiredOffset] = desc;
            }
            return orderedDescriptors;
        }

        DXILReflectionData getReflectionData(ShaderCode** shaders, size_t shaderCount,DescriptorIdentity(*identify)(const DescriptorIdentityParameters&,void*), void* identifyData)
        {
            //TODO: move this out of here, I shouldn't create and destroy this every time
            Microsoft::WRL::ComPtr<IDxcUtils> dxilUtils = nullptr;
            DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxilUtils));

            std::unordered_map<uint32_t,std::unordered_map<std::string,DXPositionalDescriptor>> descriptorGroups;

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
                    uint32_t descriptorIndex = bindDesc.BindPoint;
                    if (type == Descriptor::Type::UNKNOWN)
                    {
                        throw std::runtime_error(std::string("Unsupported descriptor type for variable \"")+bindDesc.Name+"\" in dxil shader code\"");
                    }
                    if (identify!=nullptr)
                    {
                        DescriptorIdentityParameters renameParameters{};
                        renameParameters.language = ShaderCode::CodeLanguage::DXIL;
                        renameParameters.originalName = name;
                        renameParameters.descriptorGroupIndex = bindDesc.Space;
                        renameParameters.type = type;
                        renameParameters.dimension = dimension;
                        renameParameters.arrayDepth = bindDesc.BindCount;
                        renameParameters.platformSpecificBindingIndex = bindDesc.BindPoint;
                        auto identity = identify(renameParameters,identifyData);
                        name = identity.name;
                        descriptorIndex = identity.index;
                    }
                    auto group = descriptorGroups.find(bindDesc.Space);
                    if (group == descriptorGroups.end())
                    {
                        group = descriptorGroups.insert(std::pair<uint32_t,std::unordered_map<std::string,DXPositionalDescriptor>>(bindDesc.Space,std::unordered_map<std::string,DXPositionalDescriptor>())).first;
                    }
                    auto descriptor = group->second.find(bindDesc.Name);
                    if (descriptor == group->second.end())
                    {
                        group->second.insert(std::pair<std::string,DXPositionalDescriptor>(bindDesc.Name,{Descriptor(name,type,dimension,bindDesc.BindCount,shader->stage()),descriptorIndex,bindDesc}));
                    }
                    else
                    {
                        if (descriptor->second.descriptor.shape().type!=type || descriptor->second.descriptor.shape().dimension!=dimension || descriptor->second.descriptor.shape().arrayDepth!=bindDesc.BindCount)
                        {
                            throw std::runtime_error("Incompatible descriptor sets between shader stages");
                        }
                        descriptor->second.descriptor = Descriptor(name,type,dimension,bindDesc.BindCount,shader->stage() | descriptor->second.descriptor.shape().visibleStages);
                        if (descriptor->second.desiredOffset == UINT32_MAX && descriptorIndex != UINT32_MAX)
                        {
                            descriptor->second.desiredOffset = descriptorIndex;
                        }
                    }
                }
            }

            std::vector<std::vector<CD3DX12_DESCRIPTOR_RANGE1>> descriptorGroupRanges(descriptorGroups.size());
            std::vector<std::vector<DXPositionalDescriptor>> descriptorGroupDescriptors(descriptorGroups.size());
            for (int i = 0; i < descriptorGroups.size(); i++)
            {
                auto& group = descriptorGroups[i];
                auto& groupRanges =  descriptorGroupRanges[i];
                auto& groupDescriptors = descriptorGroupDescriptors[i];

                groupDescriptors = std::move(DXILExtractUnorderedDescriptors(group));
                groupRanges.resize(groupDescriptors.size());
                for (int j=0; j<groupDescriptors.size(); j++)
                {
                    auto& currentDescriptor = groupDescriptors[j];
                    groupRanges[j].Init(
                        rangeType(currentDescriptor.descriptor.shape().type),
                        1,
                        j,
                        i,//maybe should be reflected descriptor space?
                        D3D12_DESCRIPTOR_RANGE_FLAG_NONE,//Not sure about this, may need to be volatile https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_descriptor_range_flags
                        currentDescriptor.descriptorCountFromTableRoot);
                    int x=0;
                }
            }

            std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters(descriptorGroupRanges.size());
            for (int i = 0; i < descriptorGroupRanges.size(); i++)
            {
                auto& parameter = rootParameters[i];
                auto& descRange = descriptorGroupRanges[i];
                parameter.InitAsDescriptorTable(descRange.size(),descRange.data());
            }

            CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription(rootParameters.size(),rootParameters.data());
            Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
            Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
            auto result = D3D12SerializeVersionedRootSignature(&rootSignatureDescription,&serializedRootSig,&errorBlob);
            if (result != S_OK)
            {
                std::string error = "Failed to serialize root signature";
                if (errorBlob)
                {
                    error = (static_cast<const char*>(errorBlob->GetBufferPointer()));
                }
                throw std::runtime_error(error);
            }

            Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
            result = dx12::DX12GraphicsCard::selected()->device()->CreateRootSignature(0,serializedRootSig->GetBufferPointer(),serializedRootSig->GetBufferSize(),IID_PPV_ARGS(&rootSignature));
            if (result != S_OK)
            {
                throw std::runtime_error("Failed to create root signature");
            }

            //if identification was applied
            if (identify!=nullptr)
            {
                for (auto i=0; i< descriptorGroupDescriptors.size(); i++)
                {
                    auto& descriptors = descriptorGroupDescriptors[i];
                    descriptors = std::move(DXILExtractOrderedDescriptors(descriptors));
                }
            }

            //TODO: actually use the descriptors
            std::vector<DXILDescriptorGroupReflectionData> groupReflectionData(descriptorGroupDescriptors.size());
            for (auto i=0; i< groupReflectionData.size(); i++)
            {
                auto& reflectionData = groupReflectionData[i];
                auto& computedData = descriptorGroupDescriptors[i];
                reflectionData.groupIndex = i;
                reflectionData.descriptors.resize(computedData.size());
                reflectionData.descriptorOffsets.resize(computedData.size());
                for (int j=0; j<computedData.size(); j++)
                {
                    reflectionData.descriptors[j] = computedData[j].descriptor;
                    if (computedData[j].descriptor.shape().type != slag::Descriptor::Type::SAMPLER)
                    {
                        reflectionData.descriptorOffsets[j] = computedData[j].descriptorCountFromTableRoot * dx12::DX12GraphicsCard::selected()->resourceDescriptorSize();
                    }
                    else
                    {
                        reflectionData.descriptorOffsets[j] = computedData[j].descriptorCountFromTableRoot * dx12::DX12GraphicsCard::selected()->samplerDescriptorSize();
                    }
                }
            }
            return DXILReflectionData
            {
                .rootSignature = rootSignature,
                .groups = std::move(groupReflectionData),
                //.bufferLayouts = ,
                //.texelBufferDescriptions = ,
                //.entryPointXDim = ,
                //.entryPointYDim = ,
                //.entryPointZDim = ,
            };
        }
    }
} // slag
