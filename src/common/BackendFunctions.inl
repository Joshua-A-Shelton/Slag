// Every backend entry point, in one list (ADR 0065). Each line expands to a
// function-pointer member of BackendFunctions and, in direct-dispatch builds,
// to a declaration in the backend's namespace.
//
// SLAG_BACKEND_FN(return type, name, (parameters))

SLAG_BACKEND_FN(Result<InstanceCommon*>, createInstance, (const InstanceDesc& desc, const HostAllocator& allocator))
SLAG_BACKEND_FN(void, destroyInstance, (InstanceCommon * instance))

SLAG_BACKEND_FN(Result<void>, initializeDevice, (GraphicsDeviceCommon * device, const GraphicsDeviceInitDesc& desc))
SLAG_BACKEND_FN(void, shutdownDevice, (GraphicsDeviceCommon * device))
SLAG_BACKEND_FN(MemoryBudget, memoryBudget, (const GraphicsDeviceCommon* device))
SLAG_BACKEND_FN(Result<void>, deviceWaitIdle, (GraphicsDeviceCommon * device))
SLAG_BACKEND_FN(Result<void>, queueWaitIdle, (CommandQueueCommon * queue))

SLAG_BACKEND_FN(Result<SemaphoreCommon*>, createSemaphore, (GraphicsDeviceCommon * device, const SemaphoreDesc& desc))
SLAG_BACKEND_FN(void, destroySemaphore, (GraphicsDeviceCommon * device, SemaphoreCommon* semaphore))
SLAG_BACKEND_FN(uint64_t, semaphoreValue, (const SemaphoreCommon* semaphore))
SLAG_BACKEND_FN(Result<void>, signalSemaphore, (SemaphoreCommon * semaphore, uint64_t value))
SLAG_BACKEND_FN(Result<void>, waitSemaphores,
                (GraphicsDeviceCommon * device, std::span<const SemaphoreValue> values, WaitMode mode,
                 uint64_t timeoutNs))

SLAG_BACKEND_FN(Result<MemoryBlockCommon*>, createMemoryBlock,
                (GraphicsDeviceCommon * device, const MemoryBlockDesc& desc))
SLAG_BACKEND_FN(void, destroyMemoryBlock, (GraphicsDeviceCommon * device, MemoryBlockCommon* memoryBlock))

// Placed in desc.placement.memoryBlock when set; otherwise in its own memory.
SLAG_BACKEND_FN(Result<BufferCommon*>, createBuffer, (GraphicsDeviceCommon * device, const BufferDesc& desc))
SLAG_BACKEND_FN(void, destroyBuffer, (GraphicsDeviceCommon * device, BufferCommon* buffer))
SLAG_BACKEND_FN(PlacementInfo, bufferPlacementInfo, (const GraphicsDeviceCommon* device, const BufferDesc& desc))

// Placed in info.placement.memoryBlock when set; otherwise in its own memory.
SLAG_BACKEND_FN(Result<TextureCommon*>, createTexture, (GraphicsDeviceCommon * device, const TextureCreateInfo& info))
SLAG_BACKEND_FN(void, destroyTexture, (GraphicsDeviceCommon * device, TextureCommon* texture))
SLAG_BACKEND_FN(PlacementInfo, texturePlacementInfo,
                (const GraphicsDeviceCommon* device, const TextureCreateInfo& info))

// desc.format is resolved (never Undefined).
SLAG_BACKEND_FN(Result<AttachmentViewCommon*>, createAttachmentView,
                (GraphicsDeviceCommon * device, const AttachmentViewDesc& desc))
SLAG_BACKEND_FN(void, destroyAttachmentView, (GraphicsDeviceCommon * device, AttachmentViewCommon* view))

SLAG_BACKEND_FN(Result<PipelineCommon*>, createGraphicsPipeline,
                (GraphicsDeviceCommon * device, const GraphicsPipelineDesc& desc))
SLAG_BACKEND_FN(Result<PipelineCommon*>, createComputePipeline,
                (GraphicsDeviceCommon * device, const ComputePipelineDesc& desc))
SLAG_BACKEND_FN(void, destroyPipeline, (GraphicsDeviceCommon * device, PipelineCommon* pipeline))

SLAG_BACKEND_FN(void, writeBufferDescriptor,
                (GraphicsDeviceCommon * device, uint32_t index, const BufferDescriptorInfo& info))
SLAG_BACKEND_FN(void, writeTextureDescriptor,
                (GraphicsDeviceCommon * device, uint32_t index, const TextureDescriptorInfo& info))
SLAG_BACKEND_FN(void, writeSamplerDescriptor,
                (GraphicsDeviceCommon * device, uint32_t index, const SamplerDescriptor& descriptor))

SLAG_BACKEND_FN(Result<CommandBufferCommon*>, createCommandBuffer,
                (GraphicsDeviceCommon * device, const CommandBufferDesc& desc))
SLAG_BACKEND_FN(void, destroyCommandBuffer, (GraphicsDeviceCommon * device, CommandBufferCommon* commandBuffer))
SLAG_BACKEND_FN(Result<void>, beginCommandBuffer, (CommandBufferCommon * commandBuffer))
SLAG_BACKEND_FN(Result<void>, endCommandBuffer, (CommandBufferCommon * commandBuffer))
SLAG_BACKEND_FN(Result<void>, resetCommandBuffer, (CommandBufferCommon * commandBuffer))
SLAG_BACKEND_FN(void, cmdBindPipeline, (CommandBufferCommon * commandBuffer, PipelineCommon* pipeline))
// width, height, and layerCount: the attachments' shared extent.
SLAG_BACKEND_FN(void, cmdBeginRendering,
                (CommandBufferCommon * commandBuffer, const RenderingDesc& desc, uint32_t width, uint32_t height,
                 uint32_t layerCount))
SLAG_BACKEND_FN(void, cmdEndRendering, (CommandBufferCommon * commandBuffer))
SLAG_BACKEND_FN(void, cmdBindVertexBuffers,
                (CommandBufferCommon * commandBuffer, uint32_t firstSlot, std::span<const VertexBufferBinding> bindings))
SLAG_BACKEND_FN(void, cmdBindIndexBuffer,
                (CommandBufferCommon * commandBuffer, BufferCommon* buffer, uint64_t offset, uint64_t size,
                 IndexFormat format))
SLAG_BACKEND_FN(void, cmdSetViewports, (CommandBufferCommon * commandBuffer, std::span<const Viewport> viewports))
SLAG_BACKEND_FN(void, cmdSetScissors, (CommandBufferCommon * commandBuffer, std::span<const Rect> scissors))
SLAG_BACKEND_FN(void, cmdSetBlendConstant, (CommandBufferCommon * commandBuffer, const float constant[4]))
SLAG_BACKEND_FN(void, cmdSetStencilReference, (CommandBufferCommon * commandBuffer, uint8_t reference))
SLAG_BACKEND_FN(void, cmdSetDepthBounds, (CommandBufferCommon * commandBuffer, float minDepth, float maxDepth))
SLAG_BACKEND_FN(void, cmdDraw,
                (CommandBufferCommon * commandBuffer, uint32_t vertexCount, uint32_t instanceCount,
                 uint32_t firstVertex, uint32_t firstInstance))
// countBuffer is null for a fixed count of maxCount.
SLAG_BACKEND_FN(void, cmdIndirect,
                (CommandBufferCommon * commandBuffer, IndirectKind kind, BufferCommon* argumentBuffer,
                 uint64_t argumentOffset, uint32_t maxCount, BufferCommon* countBuffer, uint64_t countOffset))
SLAG_BACKEND_FN(void, cmdDrawIndexed,
                (CommandBufferCommon * commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                 int32_t vertexOffset, uint32_t firstInstance))
SLAG_BACKEND_FN(void, cmdPushConstants,
                (CommandBufferCommon * commandBuffer, const void* data, uint32_t size, uint32_t offset))
SLAG_BACKEND_FN(void, cmdSetConstantBuffer, (CommandBufferCommon * commandBuffer, uint64_t address, uint32_t size))
SLAG_BACKEND_FN(void, cmdDispatch, (CommandBufferCommon * commandBuffer, uint32_t x, uint32_t y, uint32_t z))
SLAG_BACKEND_FN(void, cmdBarrier,
                (CommandBufferCommon * commandBuffer, std::span<const GlobalBarrier> globalBarriers,
                 std::span<const BufferBarrier> bufferBarriers, std::span<const TextureBarrier> textureBarriers))
SLAG_BACKEND_FN(void, cmdCopyBuffer,
                (CommandBufferCommon * commandBuffer, BufferCommon* destination, uint64_t destinationOffset,
                 BufferCommon* source, uint64_t sourceOffset, uint64_t size))
SLAG_BACKEND_FN(void, cmdResolveTexture,
                (CommandBufferCommon * commandBuffer, TextureCommon* destination, uint32_t destinationMip,
                 uint32_t destinationLayer, TextureCommon* source, uint32_t sourceLayer))
SLAG_BACKEND_FN(void, cmdCopyTexture,
                (CommandBufferCommon * commandBuffer, const TextureLocation& destination, const TextureLocation& source,
                 uint32_t width, uint32_t height, uint32_t depth))
// toTexture: buffer -> texture; otherwise texture -> buffer.
SLAG_BACKEND_FN(void, cmdCopyBufferTexture,
                (CommandBufferCommon * commandBuffer, bool toTexture, const TextureLocation& texture,
                 const BufferLocation& buffer, uint32_t width, uint32_t height, uint32_t depth))

SLAG_BACKEND_FN(Result<void>, submit, (CommandQueueCommon * queue, const SubmitDesc& desc))

// Swapchains (ADRs 0055-0059). The backend fills textures; common code makes
// the attachment views.
SLAG_BACKEND_FN(Result<SwapchainCommon*>, createSwapchain, (GraphicsDeviceCommon * device, const SwapchainDesc& desc))
SLAG_BACKEND_FN(void, destroySwapchain, (GraphicsDeviceCommon * device, SwapchainCommon* swapchain))
SLAG_BACKEND_FN(Result<void>, reconfigureSwapchain,
                (SwapchainCommon * swapchain, const SwapchainReconfigureDesc& desc))
SLAG_BACKEND_FN(Result<uint32_t>, acquireImage, (SwapchainCommon * swapchain))
SLAG_BACKEND_FN(Result<void>, presentImage, (CommandQueueCommon * queue, SwapchainCommon* swapchain, PresentMode mode))
SLAG_BACKEND_FN(uint32_t, swapchainFormats,
                (const GraphicsDeviceCommon* device, const NativeWindow& window, std::span<SwapchainFormat> formats))
SLAG_BACKEND_FN(uint32_t, presentModes,
                (const GraphicsDeviceCommon* device, const NativeWindow& window, std::span<PresentMode> modes))
SLAG_BACKEND_FN(void, setHdrMetadata, (SwapchainCommon * swapchain, const HdrMetadata& metadata))
SLAG_BACKEND_FN(void, setMaxFrameLatency, (SwapchainCommon * swapchain, uint32_t frames))
SLAG_BACKEND_FN(Result<void>, waitForNextFrame, (SwapchainCommon * swapchain, uint64_t timeoutNs))
