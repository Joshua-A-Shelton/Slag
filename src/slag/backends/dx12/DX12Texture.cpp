#include "DX12Texture.h"

#include "DX12Backend.h"
#include "DX12GraphicsCard.h"
#include "slag/exceptions/ResourceCreationError.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace dx12
    {
        DX12Texture::DX12Texture(
            DX12GraphicsCard* card,
            uint32_t width,
            PixelFormat format,
            TextureUsageFlags usage,
            uint32_t mipLevels,
            uint32_t layers)
        {
            auto formatInfo = card->formatProperties(format);
            if (formatInfo.tiling == TextureTiling::UNSUPPORTED)
            {
                throw ResourceCreationError("Given Pixel Format is not compatible on this graphics card");
            }

            _graphicsCard = card;
            _width = width;
            _format = format;
            _usage = usage;
            _mipLevels = mipLevels;
            _layers = layers;
            _type = TextureType::ONE_DIMENSIONAL;

            construct(D3D12_RESOURCE_DIMENSION_TEXTURE1D);
        }

        DX12Texture::DX12Texture(DX12GraphicsCard* card, uint32_t width, uint32_t height, PixelFormat format,
            TextureUsageFlags usage, uint32_t mipLevels, SampleCount sampleCount, uint32_t layers)
        {
            auto formatInfo = card->formatProperties(format);
            if (formatInfo.tiling == TextureTiling::UNSUPPORTED)
            {
                throw ResourceCreationError("Given Pixel Format is not compatible on this graphics card");
            }

            _graphicsCard = card;
            _width = width;
            _height = height;
            _format = format;
            _usage = usage;
            _mipLevels = mipLevels;
            _sampleCount = sampleCount;
            _layers = layers;
            _type = TextureType::TWO_DIMENSIONAL;

            construct(D3D12_RESOURCE_DIMENSION_TEXTURE2D);
        }

        DX12Texture::DX12Texture(DX12GraphicsCard* card, uint32_t width, uint32_t height, uint32_t depth,
            PixelFormat format, TextureUsageFlags usage, uint32_t mipLevels)
        {
            auto formatInfo = card->formatProperties(format);
            if (formatInfo.tiling == TextureTiling::UNSUPPORTED)
            {
                throw ResourceCreationError("Given Pixel Format is not compatible on this graphics card");
            }

            _graphicsCard = card;
            _width = width;
            _height = height;
            _depth = depth;
            _format = format;
            _usage = usage;
            _mipLevels = mipLevels;
            _type = TextureType::THREE_DIMENSIONAL;

            construct(D3D12_RESOURCE_DIMENSION_TEXTURE3D);
        }

        DX12Texture::DX12Texture(DX12GraphicsCard* card, PixelFormat format, TextureUsageFlags usage,
            uint32_t dimension, uint32_t mipLevels, uint32_t arrayDepth)
        {
            SLAG_ASSERT(dimension > 0 && "Texture must have a dimension of at least 1");
            auto formatInfo = card->formatProperties(format);
            if (formatInfo.tiling == TextureTiling::UNSUPPORTED)
            {
                throw ResourceCreationError("Given Pixel Format is not compatible on this graphics card");
            }

            _graphicsCard = card;
            _width = dimension;
            _height = dimension;
            _format = format;
            _usage = usage;
            _mipLevels = mipLevels;
            _layers = arrayDepth * 6;
            _type = TextureType::CUBE_MAP;

            construct(D3D12_RESOURCE_DIMENSION_TEXTURE2D);
        }

        DX12Texture::DX12Texture(DX12Texture&& from) noexcept
        {
            move(from);
        }

        DX12Texture& DX12Texture::operator=(DX12Texture&& from) noexcept
        {
            move(from);
            return *this;
        }

        DX12Texture::~DX12Texture()
        {
            if (_texture)
            {
                _texture->Release();
            }
            if (_allocation)
            {
                _allocation->Release();
            }
        }

        uint32_t DX12Texture::width() const
        {
            return _width;
        }

        uint32_t DX12Texture::height() const
        {
            return _height;
        }

        uint32_t DX12Texture::depth() const
        {
            return _depth;
        }

        uint32_t DX12Texture::layers() const
        {
            return _layers;
        }

        uint32_t DX12Texture::mipLevels() const
        {
            return _mipLevels;
        }

        PixelFormat DX12Texture::format() const
        {
            return _format;
        }

        SampleCount DX12Texture::sampleCount() const
        {
            return _sampleCount;
        }

        TextureType DX12Texture::type() const
        {
            return _type;
        }

        TextureUsageFlags DX12Texture::usage() const
        {
            return _usage;
        }

        GraphicsCard* DX12Texture::graphicsCard()
        {
            return _graphicsCard;
        }

        void* DX12Texture::userData()
        {
            return _userData;
        }

        void DX12Texture::setUserData(void* userData)
        {
            _userData = userData;
        }

        void DX12Texture::move(DX12Texture& from)
        {
            std::swap(_graphicsCard,from._graphicsCard);
            std::swap(_texture,from._texture);
            std::swap(_allocation,from._allocation);
            std::swap(_userData,from._userData);
            _format = from._format;
            _usage = from._usage;
            _sampleCount = from._sampleCount;
            _type = from._type;
            _width = from._width;
            _height = from._height;
            _depth = from._depth;
            _layers = from._layers;
            _mipLevels = from._mipLevels;
        }

        void DX12Texture::construct(D3D12_RESOURCE_DIMENSION dimension)
        {
            SLAG_ASSERT(_width>0 && "Texture must have a width of at least 1");
            SLAG_ASSERT(_height>0 && "Texture must have a height of at least 1");
            SLAG_ASSERT(_depth>0 && "Texture must have a depth of at least 1");
            SLAG_ASSERT(_layers>0 && "Texture must have a layer count of at least 1");
            SLAG_ASSERT(_mipLevels>0 && "Texture must have a mip level count of at least 1");
            SLAG_ASSERT((_usage & (TextureUsageFlags::COLOR_TARGET | TextureUsageFlags::DEPTH_STENCIL_TARGET)) != (TextureUsageFlags::COLOR_TARGET | TextureUsageFlags::DEPTH_STENCIL_TARGET) && "Texture cannot be both a color target and a depth/stencil target");
            SLAG_ASSERT(((_mipLevels > 1 && _sampleCount == SampleCount::ONE) || (_sampleCount != SampleCount::ONE && _mipLevels == 1) || (_mipLevels == 1 && _sampleCount == SampleCount::ONE)) && "Texture cannot have both multiple mip levels and have a sample count greater than one");

            D3D12_RESOURCE_DESC resourceDesc = {};
            resourceDesc.Dimension = dimension;
            resourceDesc.Alignment = 0;
            resourceDesc.Width = _width;
            resourceDesc.Height = _height;
            if (_type == TextureType::THREE_DIMENSIONAL)
            {
                resourceDesc.DepthOrArraySize = _depth;
            }
           else
           {
               resourceDesc.DepthOrArraySize = _layers;
           }
            resourceDesc.MipLevels = _mipLevels;
            resourceDesc.Format = DX12Backend::nativeFormat(_format);
            resourceDesc.SampleDesc.Count = static_cast<UINT>(_sampleCount);
            resourceDesc.SampleDesc.Quality = 0;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            resourceDesc.Flags = DX12Backend::nativeTextureUsageFlags(_usage);


            auto allocInfo = _graphicsCard->device()->GetResourceAllocationInfo(0,1,&resourceDesc);
            resourceDesc.Alignment = allocInfo.Alignment;

            D3D12MA::ALLOCATION_DESC allocationDesc = {};
            allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

            _graphicsCard->allocator()->CreateResource(&allocationDesc,&resourceDesc,D3D12_RESOURCE_STATE_COMMON,nullptr,&_allocation, IID_PPV_ARGS(&_texture));
        }
    } // dx12
} // slag
