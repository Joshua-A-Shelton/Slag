#ifndef SLAG_DX12LIB_H
#define SLAG_DX12LIB_H
#include <directx/d3d12.h>
#include "DX12GraphicsCard.h"
#include "../BackEndLib.h"

namespace slag
{
    namespace dx
    {

        class DX12Lib:public lib::BackEndLib
        {
        public:
            static DX12Lib* initialize();
            static void cleanup(lib::BackEndLib* library);
            static DX12Lib* get();
            static DX12GraphicsCard* card();
            static DXGI_FORMAT format(Pixels::Format pixelFormat);
            static D3D12_RESOURCE_FLAGS usage(Texture::Usage texUsage);
            static D3D12_RESOURCE_STATES layout(Texture::Layout texLayout);
            DX12Lib(DX12GraphicsCard* card);
            ~DX12Lib();
            BackEnd identifier()override;
            GraphicsCard* graphicsCard()override;
            //Swapchain
            Swapchain* newSwapchain(PlatformData platformData, uint32_t width, uint32_t height, uint8_t backBuffers, Swapchain::PresentMode mode, Pixels::Format imageFormat)override;
            //Textures
            Texture* newTexture(void* texelData, size_t dataSize, Pixels::Format dataFormat, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Usage texUsage, Texture::Layout initializedLayout, bool generateMips)override;
            Texture* newTexture(CommandBuffer* onBuffer, void* texelData, size_t dataSize, Pixels::Format dataFormat, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Usage texUsage, Texture::Layout initializedLayout, bool generateMips)override;
            //CommandBuffers
            CommandBuffer* newCommandBuffer(GpuQueue::QueueType acceptsCommands);
            //Buffers
            Buffer* newBuffer(void* data, size_t dataSize, Buffer::Accessibility accessibility, Buffer::Usage usage)override;
            Buffer* newBuffer(size_t  bufferSize, Buffer::Accessibility accessibility, Buffer::Usage usage)override;
            //Semaphores
            Semaphore* newSemaphore(uint64_t startingValue);
            void waitFor(SemaphoreValue* values, size_t count)override;

            DX12GraphicsCard* dx12GraphicsCard();
    private:
        DX12GraphicsCard* _graphicsCard = nullptr;

        };

    } // dx
} // slag

#endif //SLAG_DX12LIB_H
