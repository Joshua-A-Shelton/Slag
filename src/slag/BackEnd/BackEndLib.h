#ifndef SLAG_BACKENDLIB_H
#define SLAG_BACKENDLIB_H
#include "../SlagLib.h"

namespace slag
{
    namespace lib
    {
        class BackEndLib
        {
        public:
            static BackEndLib* get();
            static void set(BackEndLib* library);
            virtual ~BackEndLib()=default;
            virtual BackEnd identifier()=0;
            virtual GraphicsCard* graphicsCard()=0;
            //Textures
            virtual Texture* newTexture(GpuQueue* queue,void* data, Pixels::Format dataFormat, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Usage usage, Texture::Layout initializedLayout)=0;
            //Semaphores
            virtual Semaphore* newSemaphore(uint64_t startingValue)=0;
            virtual void waitFor(SemaphoreValue* values, size_t count)=0;
        };

    }
}
#endif //SLAG_BACKENDLIB_H
