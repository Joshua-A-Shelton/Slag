#ifndef SLAG_GRAPHICSCARD_H
#define SLAG_GRAPHICSCARD_H
#include <string>
#include "Buffer.h"
#include "CommandBuffer.h"
#include "PixelFormatProperties.h"
#include "Pixels.h"
#include "SubmissionQueue.h"
#include "Texture.h"

namespace slag
{
    ///Hardware used for performing parallel computing. May or may not actually be a dedicated "Graphics Card" per se, but does support large scale parallel computation functionality
    class GraphicsCard
    {
    protected:
        GraphicsCard() = default;
    public:
        virtual ~GraphicsCard() = default;
        ///Name of Graphics Card
        [[nodiscard]] virtual std::string name()const=0;
        ///Total size in bytes of video memory
        [[nodiscard]] virtual uint64_t videoMemory()const=0;
        ///If the entire range of GPU memory is accessible by the CPU (true if under Unified Memory Access, or resizeable BAR is enabled)
        [[nodiscard]] virtual bool cacheCoherentSharedMemory()const=0;
        ///The largest possible size in bytes of a buffer with BufferShaderAccess::UNIFORM
        [[nodiscard]] virtual uint64_t maxShaderAccessUniformBufferSize()const=0;
        /**
         * Checks if the graphics card supports a given format
         * @param format Format to see if is supported
         * @return
         */
        [[nodiscard]] virtual PixelFormatProperties formatProperties(PixelFormat format)const=0;
        ///Queue that supports graphics operations
        [[nodiscard]] virtual SubmissionQueue* graphicsQueue()=0;
        ///Queue that supports compute operations (dedicated if supported, will fall back to a higher queue if not)
        [[nodiscard]] virtual SubmissionQueue* computeQueue()=0;
        ///Queue that supports transfer operations (dedicated if supported, will fall back to a higher queue if not)
        [[nodiscard]] virtual SubmissionQueue* transferQueue()=0;

        /**
         * Defragment video memory. This is a blocking call, and will not return until defragmentation is complete
         * @param waitFor Array of SemaphoreValues to wait for before defragmentation begins
         * @param waitCount Number of SemaphoreValues in waitFor array
         * @param signal Array of SemaphoreValues to signal after defragmentation ends
         * @param signalCount Number of SemaphoreValues in signal array
         * @param targetBytes Number of bytes to attempt to defragment before finishing, or 0 for a full defragmenation
         * @return number of bytes defragmented
         */
        virtual uint64_t defragmentMemory(SemaphoreValue* waitFor, uint32_t waitCount, SemaphoreValue* signal, uint32_t signalCount, uint64_t targetBytes=0)=0;

        //Command Buffers
        /**
         * Create a new command buffer
         * @param type What kind of operations this buffer can perform
         * @return
         * @throws slag::ResourceCreationError if unable to allocate a new command buffer
         */
        [[nodiscard]] virtual CommandBuffer* newCommandBuffer(QueueType type)=0;

        //Semaphores
        /**
         * Create a new Semaphore
         * @param initialValue initial value of the semaphore
         * @return
         * @throws slag::ResourceCreationError if unable to allocate a new command buffer
         */
        [[nodiscard]] virtual Semaphore* newSemaphore(uint64_t initialValue=0)=0;

        //Buffers
        /**
         * Allocate a new buffer
         * @param size Size in bytes of the buffer
         * @param shaderAccess Data access permissions for shaders
         * @param cpuAccess Data access permissions for the cpu
         * @return
         * @throws slag::ResourceCreationError if unable to allocate a new buffer
         */
        [[nodiscard]] virtual Buffer* newBuffer(
            uint64_t size,
            BufferMemoryType shaderAccess = BufferMemoryType::GENERAL,
            BufferCPUAccess cpuAccess = BufferCPUAccess::NONE)=0;

        //Textures

        /**
         * Allocate a new 1D texture
         * @param width Number of texels in width
         * @param format Format of texels in texture
         * @param usage Capabilities of this texture
         * @param mipLevels Number of downsized LOD levels
         * @param layers Array depth of texture
         * @return
         * @throws slag::ResourceCreationError if unable to allocate a new texture
         */
        [[nodiscard]] virtual Texture* newTexture(
            uint32_t width,
            PixelFormat format,
            TextureUsageFlags usage,
            uint32_t mipLevels = 1,
            uint32_t layers = 1)=0;

        /**
         * Allocate a new 2D texture
         * @param width Number of texels in width
         * @param height Number of texels in height
         * @param format Format of texels in texture
         * @param usage Capabilities of this texture
         * @param mipLevels Number of downsized LOD levels
         * @param layers Array depth of texture
         * @return
         * @throws slag::ResourceCreationError if unable to allocate a new texture
         */
        [[nodiscard]] virtual Texture* newTexture(
            uint32_t width,
            uint32_t height,
            PixelFormat format,
            TextureUsageFlags usage,
            uint32_t mipLevels = 1,
            SampleCount = SampleCount::ONE,
            uint32_t layers = 1)=0;

        /**
         * Allocate a new 3D texture
         * @param width Number of texels in width
         * @param height Number of texels in height
         * @param depth Number of texels in depth
         * @param format Format of texels in texture
         * @param usage Capabilities of this texture
         * @param mipLevels Number of downsized LOD levels
         * @return
         * @throws slag::ResourceCreationError if unable to allocate a new texture
         */
        [[nodiscard]] virtual Texture* newTexture(
            uint32_t width,
            uint32_t height,
            uint32_t depth,
            PixelFormat format,
            TextureUsageFlags usage,
            uint32_t mipLevels = 1)=0;

        /**
         * Allocate a new Cube Texture
         * @param dimension Number of texels in width and height
         * @param format Format of texels in texture
         * @param usage Capabilities of this texture
         * @param mipLevels Number of downsized LOD levels
         * @param arrayDepth number of cubes in the array (Total layers will be arrayDepth * 6)
         * @return
         * @throws slag::ResourceCreationError if unable to allocate a new texture
         */
        [[nodiscard]] virtual Texture* newTextureCube(
            uint32_t dimension,
            PixelFormat format,
            TextureUsageFlags usage,
            uint32_t mipLevels = 1,
            SampleCount = SampleCount::ONE,
            uint32_t arrayDepth = 1
            )=0;
    };
} // slag

#endif //SLAG_GRAPHICSCARD_H
