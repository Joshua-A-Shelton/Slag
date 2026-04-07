#include "DX12ShaderModule.h"

#include "DX12GraphicsCard.h"
#include "slag/exceptions/InvalidShaderCodeError.h"
#include "slag/shader-reflection/dxil12/DXILShaderReflector.h"
#include "slag/shader-reflection/spirv/SPIRVShaderReflector.h"

namespace slag
{
    namespace dx12
    {
        DX12ShaderModule::DX12ShaderModule(DX12GraphicsCard* card, ShaderLanguage language, void* data,
            uint32_t dataLength)
        {
            _language = language;
            switch (_language)
            {
            case ShaderLanguage::DXIL:
                _metaData = DXILShaderReflector::GetMetaData(data, dataLength);
                break;
            case ShaderLanguage::SPIRV:
                _metaData = SPIRVShaderReflector::GetMetaData(data, dataLength);
                break;
            default:
                throw InvalidShaderCodeError("Shader language is not supported for this backend");
            }
            _data.resize(dataLength);
            memcpy(_data.data(), data, dataLength);
            _graphicsCard = card;
            _language = language;

        }

        ShaderLanguage DX12ShaderModule::shaderLanguage() const
        {
            return _language;
        }

        const ShaderMetaData& DX12ShaderModule::metaData()
        {
            return _metaData;
        }

        GraphicsCard* DX12ShaderModule::graphicsCard() const
        {
            return _graphicsCard;
        }
    } // dx12
} // slag
