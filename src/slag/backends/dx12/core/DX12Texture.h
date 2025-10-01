#ifndef SLAG_DX12TEXTURE_H
#define SLAG_DX12TEXTURE_H
#include <slag/Slag.h>
#include <d3d12.h>

#include "D3D12MemAlloc.h"

namespace slag
{
    namespace dx12
    {
        class DX12Texture: public Texture
        {
        public:
            DX12Texture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount);
            DX12Texture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount, void* texelData, uint64_t texelDataLength, TextureBufferMapping* mappings, uint32_t mappingCount);
            ~DX12Texture()override;
            DX12Texture(const DX12Texture&) = delete;
            DX12Texture& operator=(const DX12Texture&) = delete;
            DX12Texture(DX12Texture&& from);
            DX12Texture& operator=(DX12Texture&& from);
            ///What kind of type this texture is
            virtual Type type()override;
            ///What kind of usage does this texture support
            virtual UsageFlags usageFlags()override;
            ///Samples used in multisampling
            virtual SampleCount sampleCount()override;
            ///Width in texels
            virtual uint32_t width()override;
            ///Height in pixels
            virtual uint32_t height()override;
            ///Number of depth slices in 3D textures, 1 in everything else
            virtual uint32_t depth()override;
            ///Number of elements in the array (1D or 2D textures), (or 6 in cubemaps, one for each face of the cube), must be 1 in 3d Textures
            virtual uint32_t layers()override;
            ///Number of mip levels (lower LOD images used in shader sampling)
            virtual uint32_t mipLevels()override;
            ///The type of texel format backing the image
            virtual Pixels::Format format()override;

            ID3D12Resource* dx12Handle()const;
        private:
            void move(DX12Texture&& from);
            void construct(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount);
            ID3D12Resource* _texture = nullptr;
            D3D12MA::Allocation* _allocation = nullptr;
            ID3D12DescriptorHeap* _heap = nullptr;
            D3D12_CPU_DESCRIPTOR_HANDLE _view{};
            Texture::Type _type;
            uint32_t _width = 0;
            uint32_t _height = 0;
            uint32_t _depth = 0;
            uint32_t _layers=1;
            uint32_t _mipLevels=1;
            Texture::SampleCount _sampleCount=SampleCount::ONE;
            Pixels::Format _format;
            UsageFlags _usage;

        };
    } // dx12
} // slag

#endif //SLAG_DX12TEXTURE_H
