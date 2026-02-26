#ifndef SLAG_DX12TEXTURE_H
#define SLAG_DX12TEXTURE_H
#include <slag/Slag.h>
#include <d3d12.h>
#include <D3D12MemAlloc.h>

namespace slag
{
    namespace dx12
    {
        class DX12GraphicsCard;
        class DX12Texture: public Texture
        {
        public:
            DX12Texture(
                DX12GraphicsCard* card,
                uint32_t width,
                PixelFormat format,
                TextureUsageFlags usage,
                uint32_t mipLevels,
                uint32_t layers);

            DX12Texture(
                DX12GraphicsCard* card,
                uint32_t width,
                uint32_t height,
                PixelFormat format,
                TextureUsageFlags usage,
                uint32_t mipLevels,
                SampleCount sampleCount,
                uint32_t layers);

            DX12Texture(
                DX12GraphicsCard* card,
                uint32_t width,
                uint32_t height,
                uint32_t depth,
                PixelFormat format,
                TextureUsageFlags usage,
                uint32_t mipLevels);

            DX12Texture(
                DX12GraphicsCard* card,
                PixelFormat format,
                TextureUsageFlags usage,
                uint32_t dimension,
                uint32_t mipLevels,
                uint32_t arrayDepth);

            DX12Texture(const DX12Texture&)=delete;
            DX12Texture& operator=(const DX12Texture&)=delete;
            DX12Texture(DX12Texture&& from) noexcept;
            DX12Texture& operator=(DX12Texture&& from) noexcept;
            ~DX12Texture()override;
            [[nodiscard]] uint32_t width()const override;
            [[nodiscard]] uint32_t height()const override;
            [[nodiscard]] uint32_t depth()const override;
            [[nodiscard]] uint32_t layers()const override;
            [[nodiscard]] uint32_t mipLevels()const override;
            [[nodiscard]] PixelFormat format()const override;
            [[nodiscard]] SampleCount sampleCount()const override;
            [[nodiscard]] TextureType type()const override;
            [[nodiscard]] TextureUsageFlags usage()const override;
            [[nodiscard]] GraphicsCard* graphicsCard()override;
            [[nodiscard]] void* userData()override;
            void setUserData(void* userData)override;

        private:
            void move(DX12Texture& from);
            void construct(D3D12_RESOURCE_DIMENSION dimension);

            DX12GraphicsCard* _graphicsCard = nullptr;
            ID3D12Resource* _texture = nullptr;
            D3D12MA::Allocation* _allocation = nullptr;
            void* _userData = nullptr;
            PixelFormat _format = PixelFormat::UNDEFINED;
            TextureUsageFlags _usage = TextureUsageFlags::NONE;
            SampleCount _sampleCount = SampleCount::ONE;
            TextureType _type = TextureType::TWO_DIMENSIONAL;
            uint32_t _width = 1, _height = 1, _depth = 1, _layers = 1, _mipLevels = 1;
        };
    } // dx12
} // slag

#endif //SLAG_DX12TEXTURE_H
