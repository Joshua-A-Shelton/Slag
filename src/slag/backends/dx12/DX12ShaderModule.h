#ifndef SLAG_DX12SHADERMODULE_H
#define SLAG_DX12SHADERMODULE_H
#include <slag/Slag.h>
namespace slag
{
    namespace dx12
    {
        class DX12GraphicsCard;

        class DX12ShaderModule: public ShaderModule
        {
        public:
            DX12ShaderModule(DX12GraphicsCard* card, ShaderLanguage language, void* data, uint32_t dataLength);
            ~DX12ShaderModule()override = default;
            ///Language this shader module is
            [[nodiscard]] ShaderLanguage shaderLanguage() const override;
            ///Metadata details about the shader itself
            [[nodiscard]] const ShaderMetaData& metaData()override;
            [[nodiscard]] DX12GraphicsCard* graphicsCard() const;
        private:
            std::vector<uint32_t> _data;
            ShaderMetaData _metaData{};
            DX12GraphicsCard* _graphicsCard = nullptr;
            ShaderLanguage _language = ShaderLanguage::SPIRV;
        };
    } // dx12
} // slag

#endif //SLAG_DX12SHADERMODULE_H
