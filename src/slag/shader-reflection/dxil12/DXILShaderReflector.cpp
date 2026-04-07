#include "DXILShaderReflector.h"

#include <bit>
#include <d3d12shader.h>
#include <wrl/client.h>
#include <dxcapi.h>
#include <map>

#include "slag/exceptions/InvalidShaderCodeError.h"
#include "slag/exceptions/NotImplemented.h"

namespace slag
{

    ShaderType getShaderType(UINT version)
    {
        switch (version)
        {
        case D3D12_SHVER_PIXEL_SHADER:
            return ShaderType::FRAGMENT;
        case D3D12_SHVER_VERTEX_SHADER:
            return ShaderType::VERTEX;
        case D3D12_SHVER_GEOMETRY_SHADER:
            return ShaderType::GEOMETRY;
        case D3D12_SHVER_COMPUTE_SHADER:
            return ShaderType::COMPUTE;
        case D3D12_SHVER_RAY_GENERATION_SHADER:
            return ShaderType::RAY_GENERATION;
        case D3D12_SHVER_ANY_HIT_SHADER:
            return ShaderType::ANY_HIT;
        case D3D12_SHVER_CLOSEST_HIT_SHADER:
            return ShaderType::CLOSEST_HIT;
        case D3D12_SHVER_MISS_SHADER:
            return ShaderType::MISS;
        case D3D12_SHVER_INTERSECTION_SHADER:
            return ShaderType::INTERSECTION;
        case D3D12_SHVER_CALLABLE_SHADER:
            return ShaderType::CALLABLE;
        case D3D12_SHVER_MESH_SHADER:
            return ShaderType::MESH;
        case D3D12_SHVER_AMPLIFICATION_SHADER:;
            return ShaderType::TASK;
        }
        throw InvalidShaderCodeError("Unknown shader type");
    }

    DescriptorType getDescriptorType(D3D_SHADER_INPUT_TYPE type, D3D_SRV_DIMENSION dimension)
    {
        switch (type)
        {
        case D3D_SIT_CBUFFER:
            return DescriptorType::UNIFORM_BUFFER;
        case D3D_SIT_TBUFFER:
            return DescriptorType::UNIFORM_TEXEL_BUFFER;
        case D3D_SIT_TEXTURE:
            if (dimension == D3D_SRV_DIMENSION_BUFFER )
            {
                return DescriptorType::UNIFORM_TEXEL_BUFFER;
            }
            return DescriptorType::SAMPLED_TEXTURE;
        case D3D_SIT_SAMPLER:
            return DescriptorType::SAMPLER;
        case D3D_SIT_UAV_RWTYPED:
            if (dimension == D3D_SRV_DIMENSION_BUFFER )
            {
                return DescriptorType::UNORDERED_ACCESS_TEXEL_BUFFER;
            }
            return DescriptorType::UNORDERED_ACCESS_TEXTURE;
        case D3D_SIT_STRUCTURED:
            return DescriptorType::UNIFORM_BUFFER;
        case D3D_SIT_UAV_RWSTRUCTURED:
            return DescriptorType::UNORDERED_ACCESS_BUFFER;
        case D3D_SIT_BYTEADDRESS:
            return DescriptorType::UNIFORM_BUFFER;
        case D3D_SIT_UAV_RWBYTEADDRESS:
            return DescriptorType::UNORDERED_ACCESS_BUFFER;
        }
        throw InvalidShaderCodeError("Invalid descriptor type");
    }

    GraphicsType getGraphicsType(D3D_REGISTER_COMPONENT_TYPE registerType, uint32_t count)
    {
        GraphicsTypeBits bits = GraphicsTypeBits::NONE;
        switch (registerType)
        {
        case D3D_REGISTER_COMPONENT_UINT32:
            bits |= GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_32_BIT;
           break;
        case D3D_REGISTER_COMPONENT_SINT32:
            bits |= GraphicsTypeBits::INTEGER_32_BIT;
            break;
        case D3D_REGISTER_COMPONENT_FLOAT32:
            bits |= GraphicsTypeBits::FLOAT_32_BIT;
            break;
        case D3D_REGISTER_COMPONENT_UINT16:
            bits |= GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_16_BIT;
            break;
        case D3D_REGISTER_COMPONENT_SINT16:
            bits |= GraphicsTypeBits::INTEGER_16_BIT;
            break;
        case D3D_REGISTER_COMPONENT_FLOAT16:
            bits |= GraphicsTypeBits::FLOAT_16_BIT;
            break;
        case D3D_REGISTER_COMPONENT_UINT64:
            bits |= GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_64_BIT;
            break;
        case D3D_REGISTER_COMPONENT_SINT64:
            bits |= GraphicsTypeBits::INTEGER_64_BIT;
            break;
        case D3D_REGISTER_COMPONENT_FLOAT64:
            bits |= GraphicsTypeBits::FLOAT_64_BIT;
            break;
        default:
            throw InvalidShaderCodeError("Invalid graphics type found in shader buffer");
        }

        switch (count)
        {
        case 1:
            break;
        case 2:
            bits |= GraphicsTypeBits::VECTOR2_BIT;
            break;
        case 3:
            bits |= GraphicsTypeBits::VECTOR3_BIT;
            break;
        case 4:
            bits |= GraphicsTypeBits::VECTOR4_BIT;
            break;
        default:
            throw InvalidShaderCodeError("Invalid graphics type found in shader buffer");
        }

        return static_cast<GraphicsType>(bits);
    }

    GraphicsType getGraphicsType(D3D12_SHADER_TYPE_DESC typeDesc)
    {
        if (typeDesc.Class == D3D_SVC_STRUCT)
        {
            return GraphicsType::STRUCT;
        }
        GraphicsTypeBits bits = GraphicsTypeBits::NONE;
        switch (typeDesc.Type)
        {
        case D3D_SVT_BOOL:
            //check if int8 or bool
            bits |= GraphicsTypeBits::INTEGER_8_BIT;
            break;
        case D3D_SVT_INT16:
            bits |= GraphicsTypeBits::INTEGER_16_BIT;
            break;
        case D3D_SVT_INT:
            bits |= GraphicsTypeBits::INTEGER_32_BIT;
            break;
        case D3D_SVT_INT64:
            bits |= GraphicsTypeBits::INTEGER_64_BIT;
            break;

        case D3D_SVT_UINT8:
            bits |= GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_8_BIT;
            break;
        case D3D_SVT_UINT16:
            bits |= GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_16_BIT;
            break;
        case D3D_SVT_UINT:
            bits |= GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_32_BIT;
            break;
        case D3D_SVT_UINT64:
            bits |= GraphicsTypeBits::UNSIGNED_BIT | GraphicsTypeBits::INTEGER_64_BIT;
            break;

        case D3D_SVT_FLOAT16:
            bits |= GraphicsTypeBits::FLOAT_16_BIT;
            break;
        case D3D_SVT_FLOAT:
            bits |= GraphicsTypeBits::FLOAT_32_BIT;
            break;
        case D3D_SVT_DOUBLE:
            bits |= GraphicsTypeBits::FLOAT_64_BIT;
            break;

        default:
            throw InvalidShaderCodeError("Invalid Graphics Type found in shader buffer");
        }

        switch (typeDesc.Class)
        {
        case D3D_SVC_SCALAR:
            break;
        case D3D_SVC_VECTOR:
            switch (typeDesc.Columns)
            {
            case 2:
                bits |= GraphicsTypeBits::VECTOR2_BIT;
                break;
            case 3:
                bits |= GraphicsTypeBits::VECTOR3_BIT;
                break;
            case 4:
                bits |= GraphicsTypeBits::VECTOR4_BIT;
                break;
            }
            break;
        case D3D_SVC_MATRIX_ROWS:
        case D3D_SVC_MATRIX_COLUMNS:
            switch (typeDesc.Columns)
            {
            case 2:
                bits |= GraphicsTypeBits::VECTOR2_BIT;
                break;
            case 3:
                bits |= GraphicsTypeBits::VECTOR3_BIT;
                break;
            case 4:
                bits |= GraphicsTypeBits::VECTOR4_BIT;
                break;
            }
            switch (typeDesc.Rows)
            {
            case 2:
                bits |= GraphicsTypeBits::MATRIX2X_BIT;
                break;
            case 3:
                bits |= GraphicsTypeBits::MATRIX3X_BIT;
                break;
            case 4:
                bits |= GraphicsTypeBits::MATRIX4X_BIT;
                break;
            }
            break;
        default:
            throw InvalidShaderCodeError("Invalid Graphics Type found in shader buffer");
        }
        return (GraphicsType)bits;
    }

    TextureDescription getTextureDescription(D3D_SRV_DIMENSION dimension)
    {
        switch (dimension)
        {
        case D3D_SRV_DIMENSION_TEXTURE1D:
            return TextureDescription(TextureType::ONE_DIMENSIONAL,false,false);
        case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
            return TextureDescription(TextureType::ONE_DIMENSIONAL,false,true);
        case D3D_SRV_DIMENSION_TEXTURE2D:
            return TextureDescription(TextureType::TWO_DIMENSIONAL,false,false);
        case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
            return TextureDescription(TextureType::TWO_DIMENSIONAL,false,true);
        case D3D_SRV_DIMENSION_TEXTURE2DMS:
            return TextureDescription(TextureType::TWO_DIMENSIONAL,true,false);
        case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
            return TextureDescription(TextureType::TWO_DIMENSIONAL,true,true);
        case D3D_SRV_DIMENSION_TEXTURE3D:
            return TextureDescription(TextureType::THREE_DIMENSIONAL,false,false);
        case D3D_SRV_DIMENSION_TEXTURECUBE:
            return TextureDescription(TextureType::CUBE_MAP,false,false);
            break;
        case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
            return TextureDescription(TextureType::CUBE_MAP,false,true);
        }
        throw InvalidShaderCodeError("Unrecognized Texture description encountered in shader buffer");
    }

    StructMember getStructMember(const std::string& name,ID3D12ShaderReflectionType* reflectionType,uint32_t parentOffset)
    {
        D3D12_SHADER_TYPE_DESC desc;
        reflectionType->GetDesc(&desc);
        auto type = getGraphicsType(desc);
        auto arraylength = desc.Elements == 0 ? 1 : desc.Elements;
        if (type == GraphicsType::STRUCT)
        {
            std::vector<StructMember> structMembers;
            structMembers.reserve(desc.Members);

            for (auto i=0; i<desc.Members; i++)
            {
                structMembers.emplace_back(getStructMember(reflectionType->GetMemberTypeName(i),reflectionType->GetMemberTypeByIndex(i), parentOffset + desc.Offset));
            }
            return StructMember(name,std::move(structMembers),arraylength,desc.Offset,parentOffset + desc.Offset);
        }
        else
        {
            return StructMember(name,type,arraylength,desc.Offset,parentOffset + desc.Offset);
        }

    }

    BufferLayout getBufferLayout(ID3D12ShaderReflectionConstantBuffer* buffer)
    {
        D3D12_SHADER_BUFFER_DESC desc;
        buffer->GetDesc(&desc);
        std::vector<StructMember> members;
        members.reserve(desc.Variables);
        for (auto i=0u; i<desc.Variables; i++)
        {
            auto bufferVar = buffer->GetVariableByIndex(i);
            D3D12_SHADER_VARIABLE_DESC var;
            bufferVar->GetDesc(&var);
            members.emplace_back(getStructMember(var.Name,bufferVar->GetType(), var.StartOffset));
        }
        return BufferLayout(std::move(members));

    }

    DescriptorMeta getDescriptorMeta(const D3D12_SHADER_INPUT_BIND_DESC& bindDesc, const Microsoft::WRL::ComPtr<ID3D12ShaderReflection>& reflection)
    {
        auto type = getDescriptorType(bindDesc.Type, bindDesc.Dimension);
        switch (type)
        {
        case DescriptorType::SAMPLER:
            return DescriptorMeta(bindDesc.Name,type,bindDesc.Space,bindDesc.BindPoint,bindDesc.BindCount,SamplerDescription{});
        case DescriptorType::SAMPLED_TEXTURE:
        case DescriptorType::UNORDERED_ACCESS_TEXTURE:
            return DescriptorMeta(bindDesc.Name,type,bindDesc.Space,bindDesc.BindPoint,bindDesc.BindCount,getTextureDescription(bindDesc.Dimension));
        case DescriptorType::UNIFORM_BUFFER:
        case DescriptorType::UNORDERED_ACCESS_BUFFER:
            if (bindDesc.Type == D3D_SIT_CBUFFER)
            {
                auto constBuffer = reflection->GetConstantBufferByName(bindDesc.Name);
                return DescriptorMeta(bindDesc.Name,type,bindDesc.Space,bindDesc.BindPoint,bindDesc.BindCount,getBufferLayout(constBuffer));
            }
            return DescriptorMeta(bindDesc.Name,type,bindDesc.Space,bindDesc.BindPoint,bindDesc.BindCount,BufferLayout(std::vector<StructMember>()));
            break;
        case DescriptorType::UNIFORM_TEXEL_BUFFER:
        case DescriptorType::UNORDERED_ACCESS_TEXEL_BUFFER:
            throw NotImplemented();
            break;
        case DescriptorType::ACCELERATION_STRUCTURE:
            throw NotImplemented();
            break;
        }
        throw InvalidShaderCodeError("Invalid Descriptor Meta found in shader");
    }

    std::vector<BindGroup> getBindGroups(const D3D12_SHADER_DESC& shaderDesc, const Microsoft::WRL::ComPtr<ID3D12ShaderReflection>& reflection)
    {

        std::map<uint32_t, std::vector<DescriptorMeta>> descriptorMetas;

        for (auto i=0; i< shaderDesc.BoundResources; i++)
        {
            D3D12_SHADER_INPUT_BIND_DESC bindDesc{};
            reflection->GetResourceBindingDesc(i,&bindDesc);

            if (descriptorMetas.contains(bindDesc.Space))
            {
                auto& metas = descriptorMetas.at(bindDesc.Space);
                metas.emplace_back(getDescriptorMeta(bindDesc,reflection));
            }
            else
            {
                std::vector<DescriptorMeta> metas;
                metas.emplace_back(getDescriptorMeta(bindDesc,reflection));
                descriptorMetas.emplace(bindDesc.Space,std::move(metas));
            }

        }
        std::vector<BindGroup> bindGroups;
        bindGroups.reserve(descriptorMetas.size());
        for (auto& kvPair : descriptorMetas)
        {
            bindGroups.emplace_back(kvPair.first,std::move(kvPair.second));
        }

        return bindGroups;
    }

    std::vector<ShaderInterfaceVariable> getInputs(const D3D12_SHADER_DESC& shaderDesc, const Microsoft::WRL::ComPtr<ID3D12ShaderReflection>& reflection)
    {
        std::vector<ShaderInterfaceVariable> inputs;
        inputs.reserve(shaderDesc.InputParameters);
        for (auto i=0u; i<shaderDesc.InputParameters; i++)
        {
            D3D12_SIGNATURE_PARAMETER_DESC paramDesc;
            reflection->GetInputParameterDesc(i,&paramDesc);
            inputs.emplace_back(ShaderInterfaceVariable(paramDesc.SemanticName,getGraphicsType(paramDesc.ComponentType,std::popcount(paramDesc.Mask)),paramDesc.Register));
        }
        return inputs;
    }

    std::vector<ShaderInterfaceVariable> getOutputs(const D3D12_SHADER_DESC& shaderDesc, const Microsoft::WRL::ComPtr<ID3D12ShaderReflection>& reflection)
    {
        std::vector<ShaderInterfaceVariable> outputs;
        outputs.reserve(shaderDesc.OutputParameters);
        for (auto i=0u; i<shaderDesc.OutputParameters; i++)
        {
            D3D12_SIGNATURE_PARAMETER_DESC paramDesc;
            reflection->GetOutputParameterDesc(i,&paramDesc);
            outputs.emplace_back(ShaderInterfaceVariable(paramDesc.SemanticName,getGraphicsType(paramDesc.ComponentType,std::popcount(paramDesc.Mask)),paramDesc.Register));
        }
        return outputs;
    }

    ShaderMetaData DXILShaderReflector::GetMetaData(void* data, uint32_t dataLength)
    {
        //TODO: move this out of here, I shouldn't create and destroy this every time
        Microsoft::WRL::ComPtr<IDxcUtils> dxilUtils = nullptr;
        DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxilUtils));

        DxcBuffer buffer
        {
            .Ptr = data,
            .Size = dataLength,
            .Encoding = 0
        };

        Microsoft::WRL::ComPtr<ID3D12ShaderReflection> shaderReflection{};
        if (dxilUtils->CreateReflection(&buffer,IID_PPV_ARGS(&shaderReflection))!=S_OK)
        {
            throw InvalidShaderCodeError("Unable to get reflection data for shader code");
        }

        D3D12_SHADER_DESC shaderDesc{};
        if (shaderReflection->GetDesc(&shaderDesc)!=S_OK)
        {
            throw InvalidShaderCodeError("Unable to get reflection data for shader code");
        }
        auto type = getShaderType((shaderDesc.Version & 0xFFFF0000)>>16);
        auto groups = getBindGroups(shaderDesc, shaderReflection);
        auto inputs = getInputs(shaderDesc, shaderReflection);
        auto outputs = getOutputs(shaderDesc, shaderReflection);

        return ShaderMetaData(type,std::move(groups),std::move(inputs),std::move(outputs));
    }
} // slag
