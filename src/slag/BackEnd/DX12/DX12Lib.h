#ifndef SLAG_DX12LIB_H
#define SLAG_DX12LIB_H
#include <d3d12.h>
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
            DX12Lib(DX12GraphicsCard* card);
            BackEnd identifier()override;
            GraphicsCard* graphicsCard()override;
            //Textures
            Texture* newTexture(GpuQueue* queue,void* data, Pixels::Format dataFormat, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Usage usage, Texture::Layout initializedLayout);
            //CommandBuffers
            CommandBuffer* newCommandBuffer(GpuQueue::QueueType acceptsCommands);
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
