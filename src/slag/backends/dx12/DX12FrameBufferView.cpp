#include "DX12FrameBufferView.h"

#include "DX12Backend.h"
#include "DX12GraphicsCard.h"
#include "DX12Texture.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace dx12
    {
        DX12FrameBufferView::DX12FrameBufferView(DX12GraphicsCard* graphicsCard, Texture* texture, uint32_t mip,
            uint32_t baseLayer, uint32_t layerCount)
        {
            SLAG_ASSERT((bool)(texture->usage() & (TextureUsageFlags::COLOR_TARGET | TextureUsageFlags::DEPTH_STENCIL_TARGET)) && "Texture must have either COLOR_TARGET or DEPTH_STENCIL_TARGET usage flags");
            SLAG_ASSERT(mip < texture->mipLevels() && baseLayer < texture->layers() && baseLayer + layerCount <= texture->layers() && "Cannot assign a texture view into resources beyond the texture bounds");

            auto dx12Texture = static_cast<DX12Texture*>(texture);
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.NumDescriptors = 1;
            if((uint8_t)(texture->usage() & TextureUsageFlags::COLOR_TARGET))
            {
                desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
                graphicsCard->device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_dx12Heap));

                D3D12_RENDER_TARGET_VIEW_DESC viewDesc =
                {
                    .Format = DX12Backend::nativeFormat(dx12Texture->format()),
                };

                if (dx12Texture->type() == TextureType::TWO_DIMENSIONAL)
                {
                    if (dx12Texture->sampleCount() == SampleCount::ONE)
                    {
                        if (dx12Texture->layers() ==1)
                        {
                            viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                            viewDesc.Texture2D.MipSlice = mip;
                            viewDesc.Texture2D.PlaneSlice = 0;
                        }
                        else
                        {
                            viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                            viewDesc.Texture2DArray.MipSlice = mip;
                            viewDesc.Texture2DArray.FirstArraySlice = baseLayer;
                            viewDesc.Texture2DArray.ArraySize = layerCount;
                            viewDesc.Texture2DArray.PlaneSlice = 0;
                        }
                    }
                    else
                    {
                        if (dx12Texture->layers() ==1)
                        {
                            viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
                        }
                        else
                        {
                            viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
                            viewDesc.Texture2DMSArray.FirstArraySlice = baseLayer;
                            viewDesc.Texture2DMSArray.ArraySize = layerCount;
                        }
                    }
                }
                if (dx12Texture->type() == TextureType::ONE_DIMENSIONAL)
                {
                    if (dx12Texture->layers() == 1)
                    {
                        viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
                        viewDesc.Texture1D.MipSlice = mip;
                    }
                    else
                    {
                        viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
                        viewDesc.Texture1DArray.MipSlice = mip;
                        viewDesc.Texture1DArray.FirstArraySlice = baseLayer;
                        viewDesc.Texture1DArray.ArraySize = layerCount;
                    }
                }
                if (dx12Texture->type() == TextureType::THREE_DIMENSIONAL)
                {
                    viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
                    viewDesc.Texture3D.MipSlice = mip;
                    viewDesc.Texture3D.FirstWSlice = baseLayer;
                    viewDesc.Texture3D.WSize = layerCount;
                }
                if (dx12Texture->type() == TextureType::CUBE_MAP)
                {
                    viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                    viewDesc.Texture2DArray.MipSlice = mip;
                    viewDesc.Texture2DArray.FirstArraySlice = baseLayer;
                    viewDesc.Texture2DArray.ArraySize = layerCount;
                    viewDesc.Texture2DArray.PlaneSlice = 0;
                }

                graphicsCard->device()->CreateRenderTargetView(dx12Texture->dx12Handle(), &viewDesc,_dx12Heap->GetCPUDescriptorHandleForHeapStart());
            }
            else if((uint8_t)(texture->usage() & TextureUsageFlags::DEPTH_STENCIL_TARGET))
            {
                desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
                graphicsCard->device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_dx12Heap));
                D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc =
                {
                    .Format = DX12Backend::nativeFormat(dx12Texture->format()),
                };


                if (dx12Texture->type() == TextureType::TWO_DIMENSIONAL)
                {
                    if (dx12Texture->sampleCount() == SampleCount::ONE)
                    {
                        if (dx12Texture->layers() ==1)
                        {
                            viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                            viewDesc.Texture2D.MipSlice = 0;
                        }
                        else
                        {
                            viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                            viewDesc.Texture2DArray.MipSlice = 0;
                            viewDesc.Texture2DArray.FirstArraySlice = baseLayer;
                            viewDesc.Texture2DArray.ArraySize = layerCount;
                        }
                    }
                    else
                    {
                        if (dx12Texture->layers() ==1)
                        {
                            viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
                        }
                        else
                        {
                            viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
                            viewDesc.Texture2DMSArray.FirstArraySlice = baseLayer;
                            viewDesc.Texture2DMSArray.ArraySize = layerCount;
                        }
                    }
                }
                else if (dx12Texture->type() == TextureType::ONE_DIMENSIONAL)
                {
                    if (layerCount == 1)
                    {
                        viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
                        viewDesc.Texture1D.MipSlice = mip;
                    }
                    else
                    {
                        viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
                        viewDesc.Texture1DArray.MipSlice = mip;
                        viewDesc.Texture1DArray.FirstArraySlice = baseLayer;
                        viewDesc.Texture1DArray.ArraySize = layerCount;
                    }
                }

                graphicsCard->device()->CreateDepthStencilView(dx12Texture->dx12Handle(), &viewDesc,_dx12Heap->GetCPUDescriptorHandleForHeapStart());
            }


            _graphicsCard=graphicsCard;
            _texture=texture;
            _mip=mip;
            _baseLayer=baseLayer;
            _layerCount=layerCount;
        }

        DX12FrameBufferView::DX12FrameBufferView(DX12FrameBufferView&& from) noexcept
        {
            move(from);
        }

        DX12FrameBufferView& DX12FrameBufferView::operator=(DX12FrameBufferView&& from) noexcept
        {
            move(from);
            return *this;
        }

        DX12FrameBufferView::~DX12FrameBufferView()
        {
            if (_dx12Heap != nullptr)
            {
                _dx12Heap->Release();
            }
        }

        Texture* DX12FrameBufferView::texture()
        {
            return _texture;
        }

        uint32_t DX12FrameBufferView::mip()
        {
            return _mip;
        }

        uint32_t DX12FrameBufferView::baseLayer()
        {
            return _baseLayer;
        }

        uint32_t DX12FrameBufferView::layerCount()
        {
            return _layerCount;
        }

        GraphicsCard* DX12FrameBufferView::graphicsCard()
        {
            return _graphicsCard;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE DX12FrameBufferView::dx12Handle() const
        {
            return _dx12Heap->GetCPUDescriptorHandleForHeapStart();
        }

        void DX12FrameBufferView::move(DX12FrameBufferView& from)
        {
            std::swap(_graphicsCard,from._graphicsCard);
            std::swap(_texture,from._texture);
            std::swap(_dx12Heap,from._dx12Heap);
            std::swap(_mip,from._mip);
            std::swap(_baseLayer,from._baseLayer);
            std::swap(_layerCount,from._layerCount);
        }
    } // dx12
} // slag