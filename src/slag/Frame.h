#ifndef SLAG_FRAME_H
#define SLAG_FRAME_H
#include <vector>
#include <mutex>
#include "CommandBuffer.h"
#include "UniformSetDataAllocator.h"

namespace slag
{
    class Frame
    {
    public:
        virtual ~Frame();
        virtual void begin()=0;
        virtual void end()=0;
        virtual CommandBuffer* getCommandBuffer()=0;
        virtual Texture* getBackBuffer()=0;
        virtual UniformBuffer* getUniformBuffer()=0;
        virtual UniformSetDataAllocator* getUniformSetDataAllocator()=0;
        virtual Texture* getTextureResource(std::string resourceName)=0;
        virtual CommandBuffer* getCommandBufferResource(std::string resourceName)=0;
        virtual UniformBuffer* getUniformBufferResource(std::string resourceName)=0;
        virtual VertexBuffer* getVertexBufferResource(std::string resourceName)=0;
        virtual IndexBuffer* getIndexBufferResource(std::string resourceName)=0;
        ///Internal use only, automatically called for resources
        void queueResourceForDeletion(void* gpuID);
    protected:
        void freeResourceReferences();
    private:
        std::mutex _freedResourcesMutex;
        std::vector<void*> _freedResourceReferences;
    };
}
#endif //SLAG_FRAME_H
