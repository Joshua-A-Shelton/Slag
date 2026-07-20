#ifndef SLAG_GRAPHICSCARD_H
#define SLAG_GRAPHICSCARD_H
#include <functional>
#include <string>
#include "Buffer.h"
#include "CommandBuffer.h"
#include "Defragmentation.h"
#include "PixelFormatProperties.h"
#include "Pixels.h"
#include "PlatformData.h"
#include "SwapChain.h"
#include "Sampler.h"
#include "SubmissionQueue.h"
#include "Texture.h"
#include "ShaderCode.h"
#include "ShaderPipeline.h"
#include "VertexDescription.h"

namespace slag
{
    struct GraphicsCardMemoryProperties
    {
        ///Total size in bytes of video memory
        uint64_t videoMemory=0;
        ///The largest possible size in bytes of a buffer with BufferShaderAccess::UNIFORM
        uint64_t maxUniformBufferSize = 0;
        ///If the entire range of GPU memory is accessible by the CPU (true if under Unified Memory Access, or resizeable BAR is enabled)
        bool cacheCoherentSharedMemory = false;
    };

    struct GraphicsCardCapabilities
    {
        ///If the graphics card is capable of calling GraphicsCard::defragmentMemory or not (should usually be true, false usually indicates the backend implementation doesn't support it, rather than the card)
        bool defragmentable = false;
        ///If the card is capable of handling ray tracing operations
        bool raytracing = false;
    };

    struct DescriptorHeapDetails
    {
        ///Size to increment when writing texture descriptors to a resource descriptor heap
        uint32_t textureDescriptorSize=0;
        ///Size to increment when writing buffer descriptors to a resource descriptor heap
        uint32_t bufferDescriptorSize=0;
        ///Size to increment when writing samplers to a sampler descriptor heap
        uint32_t samplerDescriptorSize=0;
        ///Alignment requirements for writing texture descriptors to a resource descriptor heap
        uint32_t textureDescriptorAlignment=0;
        ///Alignment requirements for writing buffer descriptors to a resource descriptor heap
        uint32_t bufferDescriptorAlignment=0;
        ///Alignment requirements for writing samplers to a sampler descriptor heap
        uint32_t samplerDescriptorAlignment=0;
        ///Maximum number of descriptors that can be stored in a resource descriptor heap
        uint32_t maxResourceDescriptors=0;
        ///Maximum number of descriptors that can be stored in a sampler descriptor heap
        uint32_t maxSamplerDescriptors=0;
        ///Back end reservation of memory for implementation of certain api features
        uint32_t resourceReservedRangeSize=0;
        ///Back end reservation of memory for implementation of certain api features
        uint32_t samplerReservedRangeSize=0;
    };
    ///Hardware used for performing parallel computing. May or may not actually be a dedicated "Graphics Card" per se, but does support large scale parallel computation functionality
    class GraphicsCard
    {
    protected:
        GraphicsCard() = default;
    public:
        virtual ~GraphicsCard() = default;
        ///Name of Graphics Card
        [[nodiscard]] virtual std::string name()const=0;
        ///Properties of the memory available to this graphics card
        [[nodiscard]] virtual const GraphicsCardMemoryProperties& memoryProperties()const=0;
        ///Capabilties this graphics card has
        [[nodiscard]] virtual const GraphicsCardCapabilities& capabilities()const=0;
        ///Limitations and descriptor sizes on descriptor heaps
        [[nodiscard]] virtual const DescriptorHeapDetails& descriptorHeapDetails()const=0;
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
         * @param targetBytes Number of bytes to attempt to defragment before finishing, or 0 for a full defragmenation
         * @param memoryMoved Function to execute when a segment of memory has been moved
         * @return number of bytes defragmented
         */
        virtual uint64_t defragmentMemory(uint64_t targetBytes=0, std::function<void(MemoryReference*)> memoryMoved = nullptr)=0;

        /**
         * Create a new swapchain
         * @param platformData Platform specific data for the swapchain
         * @param width Width of the swapchain in pixels
         * @param height Height of the swapchain in pixels
         * @param parameters Parameters for the swapchain
         * @return New swapchain object
         */
        virtual SwapChain* newSwapchain(const PlatformData& platformData, uint32_t width, uint32_t height, const SwapChainParameters& parameters)=0;

        //Shaders
        /**
         * Create a new shader pipeline consisting of a vertex shader and a fragment shader
         * @param vertexDescription The expected input vertex format
         * @param vertexShader Shader that transforms vertices
         * @param fragmentShader Shader that outputs fragments
         * @param pipelineState Additional details about how rendering should be performed
         * @param framebufferDescription Description of the framebuffer targets
         * @return
         */
        [[nodiscard]] virtual ShaderPipeline* newShaderPipeline(
            const VertexDescription& vertexDescription,
            const ShaderCode& vertexShader,
            const ShaderCode& fragmentShader,
            const PipelineState& pipelineState,
            const FramebufferDescription& framebufferDescription)=0;

        /**
         * Create a new compute shader pipeline
         * @param computeShader code for the compute shader
         * @return
         */
        [[nodiscard]] virtual ShaderPipeline* newShaderPipeline(
            ShaderCode* computeShader
            )=0;


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
         * @param cpuAccess Data access permissions for the cpu
         * @param shaderAccess Data access permissions for shaders
         * @return
         * @throws slag::ResourceCreationError if unable to allocate a new buffer
         */
        [[nodiscard]] virtual Buffer* newBuffer(
            uint64_t size,
            BufferCPUAccess cpuAccess = BufferCPUAccess::NONE,
            BufferMemoryType shaderAccess = BufferMemoryType::GENERAL)=0;

        /**
         * Allocate a new resource descriptor heap
         * @param minDescriptorCount Minium number of descriptors in the heap (heap size will be the size of the largest descriptor type * minDescriptorCount)
         * @return
         */
        [[nodiscard]] virtual ResourceDescriptorHeap* newResourceDescriptorHeap(uint32_t minDescriptorCount)=0;

        /**
         * Allocate a new sampler descriptor heap
         * @param descriptorCount Number of descriptors in the heap
         * @return
         */
        [[nodiscard]] virtual SamplerDescriptorHeap* newSamplerDescriptorHeap(uint32_t descriptorCount)=0;


        /**
         * Write a read-write buffer descriptor to a location
         * @param buffer The buffer to write the descriptor for
         * @param offset Offset into the buffer that is accessible from the GPU
         * @param length Length of the buffer that is accessible from the GPU
         * @param destination Location to write the descriptor to
         */
        virtual void writeUniformBufferDescriptor(Buffer* buffer, uint64_t offset, uint64_t length, void* destination)=0;
        /**
        * Write a uniform buffer descriptor to a location
        * @param buffer The buffer to write the descriptor for
        * @param firstElementIndex Index of the first element in the buffer
        * @param elementCount Number of elements accessible in the buffer
        * @param elementStride Size of each element in the buffer
        * @param destination Location to write the descriptor to
        */
        virtual void writeReadWriteBufferDescriptor(Buffer* buffer, uint64_t firstElementIndex,uint64_t elementCount, uint64_t elementStride, void* destination)=0;
        /**
         * Write a uniform texel buffer descriptor to a location
         * @param buffer The buffer to write the descriptor for
         * @param format Format of the texel buffer
         * @param firstElementIndex Offset into the buffer that is accessible from the GPU
         * @param elementCount Length of the buffer that is accessible from the GPU
         * @param destination Location to write the descriptor to
         */
        virtual void writeUniformTexelBuffer(Buffer* buffer, PixelFormat format, uint64_t firstElementIndex, uint64_t elementCount, void* destination)=0;
        /**
         * Write a read-write texel buffer descriptor to a location
         * @param buffer The buffer to write the descriptor for
         * @param format Format of the texel buffer
         * @param firstElementIndex Offset into the buffer that is accessible from the GPU
         * @param elementCount Length of the buffer that is accessible from the GPU
         * @param destination Location to write the descriptor to
         */
        virtual void writeReadWriteTexelBuffer(Buffer* buffer, PixelFormat format, uint64_t firstElementIndex, uint64_t elementCount, void* destination)=0;
        /**
         * Write a uniform texture descriptor to a location
         * @param texture The texture to write the descriptor for
         * @param baseMip Base mip level of the texture to write the descriptor for
         * @param mipCount Number of mip levels to write the descriptor for
         * @param baseLayer Base layer of the texture to write the descriptor for
         * @param layerCount Number of layers to write the descriptor for
         * @param destination Location to write the descriptor to
         */
        virtual void writeUniformTextureDescriptor(Texture* texture, uint32_t baseMip, uint32_t mipCount, uint32_t baseLayer, uint32_t layerCount, void* destination)=0;
        /**
         * Write a read-write texture descriptor to a location
         * @param texture The texture to write the descriptor for
         * @param mip Mip level of the texture to write the descriptor for
         * @param baseLayer Base layer of the texture to write the descriptor for
         * @param layerCount Number of layers to write the descriptor for
         * @param destination Location to write the descriptor to
         */
        virtual void writeReadWriteTextureDescriptor(Texture* texture, uint32_t mip, uint32_t baseLayer, uint32_t layerCount, void* destination)=0;
        /**
         * Write a sampler descriptor to a location
         * @param sampler The sampler to write the descriptor for
         * @param destination Location to write the descriptor to
         */
        virtual void writeSamplerDescriptor(Sampler* sampler, void* destination)=0;


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
        [[nodiscard]] virtual Texture* newTexture1D(
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
         * @param sampleCount Number of samples per texel
         * @param layers Array depth of texture
         * @return
         * @throws slag::ResourceCreationError if unable to allocate a new texture
         */
        [[nodiscard]] virtual Texture* newTexture2D(
            uint32_t width,
            uint32_t height,
            PixelFormat format,
            TextureUsageFlags usage,
            uint32_t mipLevels = 1,
            SampleCount sampleCount = SampleCount::ONE,
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
        [[nodiscard]] virtual Texture* newTexture3D(
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
         * @param arrayDepth Number of cubes in the array (Total layers will be arrayDepth * 6)
         * @return
         * @throws slag::ResourceCreationError if unable to allocate a new texture
         */
        [[nodiscard]] virtual Texture* newTextureCube(
            uint32_t dimension,
            PixelFormat format,
            TextureUsageFlags usage,
            uint32_t mipLevels = 1,
            uint32_t arrayDepth = 1
            )=0;

        /**
         * Create a new Sampler
         * @param min Filter to use when down sampling
         * @param mag Filter to use when up sampling
         * @param mip Filter to use across mip boundaries
         * @param u Access pattern across the U axis
         * @param v Access pattern across the V axis
         * @param w Access pattern across the W axis
         * @param mipLODBias Shift the default accessed mip level (eg: if a sampler would have sampled mip level 3, and mipLODBias is -1, it would sample mip level 2)
         * @param anisotrophyEnabled Enable anisotropic filtering
         * @param maxAnisotrophy Maximum anisotrophy level
         * @param comparisonFunction Comparison function to use when comparing values
         * @param borderColor Border color to use when sampling outside the texture boundaries (Only if SamplerAddressMode is CLAMP_TO_BORDER)
         * @param minLOD Minimum mip level to use
         * @param maxLOD Maximum mip level to use
         * @return
         */
        [[nodiscard]] virtual Sampler* newSampler(
                SamplerFilter min = SamplerFilter::NEAREST,
                SamplerFilter mag = SamplerFilter::NEAREST,
                SamplerFilter mip = SamplerFilter::NEAREST,
                SamplerAddressMode u = SamplerAddressMode::REPEAT,
                SamplerAddressMode v = SamplerAddressMode::REPEAT,
                SamplerAddressMode w = SamplerAddressMode::REPEAT,
                float mipLODBias = 0,
                bool anisotrophyEnabled = false,
                uint8_t  maxAnisotrophy = 0,
                ComparisonFunction comparisonFunction = ComparisonFunction::NEVER,
                Color borderColor = Color(0.0f, 0.0f, 0.0f),
                float minLOD = 0,
                float maxLOD = 1000
                )=0;
    };
} // slag

#endif //SLAG_GRAPHICSCARD_H
