#include "DXIL12Reflection.h"
#include <dxcapi.h>
#include <d3d12shader.h>
#include <map>

#include <wrl/client.h>

#include <directx/d3dx12_root_signature.h>
#include "slag/backends/dx12/DX12Backend.h"
#include "slag/backends/dx12/core/DX12GraphicsCard.h"

namespace slag
{
    namespace dxil
    {
        Descriptor::Type DXILDescriptorType(D3D_SHADER_INPUT_TYPE type, D3D_SRV_DIMENSION dimension)
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

        Descriptor::Dimension DXILDescriptorDimension(D3D_SRV_DIMENSION dimension)
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


        DXILReflectionData getReflectionData(ShaderCode** shaders, size_t shaderCount,
            DescriptorIdentity(* identify)(const DescriptorIdentityParameters&, void*), void* identifyData)
        {
            auto metaData = shaders[0]->metaData();
            throw std::runtime_error("DXIL12Reflection getReflectionData not implemented");
        }
    }
} // slag
