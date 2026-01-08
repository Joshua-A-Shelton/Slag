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

        struct DXPositionalDescriptor
        {
            Descriptor descriptor;
            uint32_t desiredOffset;
            D3D12_SHADER_INPUT_BIND_DESC reflectedBinding;
        };

        std::vector<Descriptor> DXILOrderDescriptors(
            const std::vector<Descriptor>& uniformBuffers,
            const std::vector<Descriptor>& uniformTexelBuffers,
            const std::vector<Descriptor>& storageBuffers,
            const std::vector<Descriptor>& storageTexelBuffers,
            const std::vector<Descriptor>& sampledTextures,
            const std::vector<Descriptor>& storageTextures,
            const std::vector<Descriptor>& samplers,
            const std::vector<Descriptor>& accelerationStructures)
        {
            std::vector<Descriptor> descriptors;
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
            return descriptors;
        }

        std::vector<Descriptor> DXILExtractUnorderedDescriptors(const std::unordered_map<std::string, DXPositionalDescriptor>& descriptors)
        {

            std::vector<Descriptor> uniformBuffers;
            std::vector<Descriptor> uniformTexelBuffers;
            std::vector<Descriptor> storageBuffers;
            std::vector<Descriptor> storageTexelBuffers;
            std::vector<Descriptor> sampledTextures;
            std::vector<Descriptor> storageTextures;
            std::vector<Descriptor> samplers;
            std::vector<Descriptor> accelerationStructures;
            for (auto& entry : descriptors)
            {
                switch (entry.second.descriptor.shape().type)
                {
                    case Descriptor::Type::UNKNOWN:
                        throw std::runtime_error("Unknown descriptor type");
                        break;
                case Descriptor::Type::SAMPLER:
                    samplers.push_back(entry.second.descriptor);
                    break;
                case Descriptor::Type::SAMPLED_TEXTURE:
                    sampledTextures.push_back(entry.second.descriptor);
                    break;
                case Descriptor::Type::STORAGE_TEXTURE:
                    storageTextures.push_back(entry.second.descriptor);
                    break;
                case Descriptor::Type::UNIFORM_TEXEL_BUFFER:
                    uniformTexelBuffers.push_back(entry.second.descriptor);
                    break;
                case Descriptor::Type::STORAGE_TEXEL_BUFFER:
                    storageTexelBuffers.push_back(entry.second.descriptor);
                    break;
                case Descriptor::Type::UNIFORM_BUFFER:
                    uniformBuffers.push_back(entry.second.descriptor);
                    break;
                case Descriptor::Type::STORAGE_BUFFER:
                    storageBuffers.push_back(entry.second.descriptor);
                    break;
                case Descriptor::Type::ACCELERATION_STRUCTURE:
                    accelerationStructures.push_back(entry.second.descriptor);
                    break;
                }
            }
            return DXILOrderDescriptors(uniformBuffers,uniformTexelBuffers,storageTexelBuffers,storageTexelBuffers,sampledTextures,storageTextures,samplers,accelerationStructures);
        }

        std::vector<Descriptor> DXILExtractOrderedDescriptors(const std::unordered_map<std::string, DXPositionalDescriptor>& descriptors)
        {
            std::vector<Descriptor> orderedDescriptors(descriptors.size());
            for (auto& entry : descriptors)
            {
                auto& desc = entry.second;
                orderedDescriptors[desc.desiredOffset] = desc.descriptor;
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
            std::vector<DXILDescriptorGroupReflectionData> descriptorGroupReflections(descriptorGroups.size());
            //if identification was applied
            if (identify!=nullptr)
            {
                for (const auto& group : descriptorGroups)
                {
                    descriptorGroupReflections[group.first] = DXILDescriptorGroupReflectionData{group.first,DXILExtractOrderedDescriptors(group.second)};
                }
            }
            //otherwise auto sort
            else
            {
                for (const auto& group : descriptorGroups)
                {
                    descriptorGroupReflections[group.first] = DXILDescriptorGroupReflectionData{group.first,DXILExtractUnorderedDescriptors(group.second)};
                }
            }
            //TODO: actually use the descriptors
            return DXILReflectionData();
        }
    }
} // slag
