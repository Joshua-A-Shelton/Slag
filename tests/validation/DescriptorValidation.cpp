// Validation of descriptor writes and the descriptor slot allocator.

#include "ValidationFixture.h"

#include <slag/util/DescriptorAllocator.h>

using namespace slag;

TEST_CASE("buffer descriptors")
{
    ValidationFixture f;
    Buffer* buffer = f.buffer(4096);
    uint32_t outside = f.common->resourceHeapSize;
    CHECK_VALIDATION(f, "is outside the resource heap",
                     f.device->writeRawBuffer(outside, {.buffer = buffer, .size = 256}));
    CHECK_VALIDATION(f, "the address is not a multiple",
                     f.device->writeConstantBuffer(0, {.address = buffer->gpuAddress() + 4, .size = 256}));
    CHECK_VALIDATION(f, "is not a multiple of 256 up to",
                     f.device->writeConstantBuffer(0, {.address = buffer->gpuAddress(), .size = 100}));
    CHECK_VALIDATION(f, "buffer is null", f.device->writeRawBuffer(0, {.buffer = nullptr, .size = 256}));
    CHECK_VALIDATION(f, "the view ends past the buffer", f.device->writeRawBuffer(0, {.buffer = buffer, .size = 8192}));
    CHECK_VALIDATION(f, "offset is not a multiple of",
                     f.device->writeRawBuffer(0, {.buffer = buffer, .offset = 4, .size = 256}));
    CHECK_VALIDATION(f, "size must be a non-zero multiple of 4",
                     f.device->writeRWRawBuffer(0, {.buffer = buffer, .size = 6}));
    CHECK_VALIDATION(f, "stride 6 must be a non-zero multiple of 4",
                     f.device->writeStructuredBuffer(0, {.buffer = buffer, .count = 4, .stride = 6}));
    CHECK_VALIDATION(f, "offset must be a multiple of the stride",
                     f.device->writeRWStructuredBuffer(0, {.buffer = buffer, .offset = 8, .count = 4, .stride = 12}));
    CHECK_VALIDATION(f, "count 0 is 0 or exceeds",
                     f.device->writeStructuredBuffer(0, {.buffer = buffer, .count = 0, .stride = 4}));
    f.common->formatSupport[static_cast<size_t>(PixelFormat::R8G8B8A8Unorm)] &= ~FormatSupport::RWTypedBuffer;
    CHECK_VALIDATION(
        f, "can't be used in this kind of typed buffer",
        f.device->writeRWTypedBuffer(0, {.buffer = buffer, .count = 4, .format = PixelFormat::R8G8B8A8Unorm}));
    CHECK_VALIDATION(
        f, "offset is not a multiple of",
        f.device->writeTypedBuffer(0, {.buffer = buffer, .offset = 4, .count = 4, .format = PixelFormat::R32Uint}));
    CHECK_VALIDATION(f, "count 0 is 0 or exceeds limits::maxTypedBufferElements",
                     f.device->writeTypedBuffer(0, {.buffer = buffer, .count = 0, .format = PixelFormat::R32Uint}));
    CHECK_VALIDATION(f, "is outside the resource heap", f.device->clearResourceDescriptor(outside));
    f.device->destroy(buffer);
    CHECK_VALIDATION(f, "use of a destroyed Buffer", f.device->writeRawBuffer(0, {.buffer = buffer, .size = 256}));
}

TEST_CASE("texture descriptors")
{
    ValidationFixture f;
    Texture* texture = f.texture(TextureUsage::None, 64, PixelFormat::R8G8B8A8Unorm, 1, 2);
    uint32_t outside = f.common->resourceHeapSize;
    CHECK_VALIDATION(f, "is outside the resource heap", f.device->writeTexture(outside, {.texture = texture}));
    CHECK_VALIDATION(f, "texture is null", f.device->writeTexture(0, {}));
    CHECK_VALIDATION(f, "the view type doesn't match",
                     f.device->writeTexture(0, {.texture = texture, .viewType = TextureViewType::Texture3D}));
    CHECK_VALIDATION(f, "the texture was created without ShaderWrite",
                     f.device->writeRWTexture(0, {.texture = texture}));
    CHECK_VALIDATION(f, "is neither the texture's format nor castable",
                     f.device->writeTexture(0, {.texture = texture, .format = PixelFormat::R32Float}));
    CHECK_VALIDATION(f, "the mip range is outside the texture",
                     f.device->writeTexture(0, {.texture = texture, .baseMip = 2, .mipCount = 1}));
    CHECK_VALIDATION(f, "the layer range is outside the texture",
                     f.device->writeTexture(0, {.texture = texture, .baseLayer = 1}));
    Texture* msaa = f.texture(TextureUsage::RenderTarget, 64, PixelFormat::R8G8B8A8Unorm, 4);
    // MSAA textures never have ShaderWrite, so both checks fire.
    CHECK_VALIDATIONS(f, 2, "MSAA textures can't be written",
                      f.device->writeRWTexture(0, {.texture = msaa, .viewType = TextureViewType::Texture2DMS}));
    Texture* cubes =
        f.device->createTextureCubeArray({.format = PixelFormat::R8G8B8A8Unorm, .size = 16, .cubeCount = 2}).value();
    CHECK_VALIDATION(
        f, "cube views cover whole cubes",
        f.device->writeTexture(
            0, {.texture = cubes, .viewType = TextureViewType::TextureCubeArray, .baseLayer = 3, .layerCount = 6}));
    // Only reachable with a view type that doesn't match a cube array.
    CHECK_VALIDATIONS(f, 2, "the view type doesn't match",
                      f.device->writeTexture(0, {.texture = cubes, .viewType = TextureViewType::TextureCube}));
    Texture* depth = f.texture(TextureUsage::DepthStencil, 16, PixelFormat::D32FloatS8Uint);
    CHECK_VALIDATION(f, "the plane doesn't fit the format", f.device->writeTexture(0, {.texture = depth}));
    f.device->destroy(depth);
    f.device->destroy(cubes);
    f.device->destroy(msaa);
    f.device->destroy(texture);
    CHECK_VALIDATION(f, "use of a destroyed Texture", f.device->writeTexture(0, {.texture = texture}));
}

TEST_CASE("sampler descriptors")
{
    ValidationFixture f;
    uint32_t outside = f.common->samplerHeapSize;
    CHECK_VALIDATION(f, "is outside the sampler heap", f.device->writeSampler(outside, {}));
    CHECK_VALIDATION(f, "is outside the sampler heap", f.device->clearSamplerDescriptor(outside));
    CHECK_VALIDATION(f, "maxAnisotropy 0 is outside", f.device->writeSampler(0, {.maxAnisotropy = 0}));
    CHECK_VALIDATION(f, "anisotropic filtering needs every filter Linear",
                     f.device->writeSampler(0, {.minFilter = Filter::Point, .maxAnisotropy = 4}));
    CHECK_VALIDATION(f, "mipLodBias is outside", f.device->writeSampler(0, {.mipLodBias = 100.0f}));
    CHECK_VALIDATION(f, "minLod is above maxLod", f.device->writeSampler(0, {.minLod = 2.0f, .maxLod = 1.0f}));
    CHECK_VALIDATION(f, "comparison samplers use Standard reduction",
                     f.device->writeSampler(0, {.compareEnable = true, .reductionMode = ReductionMode::Minimum}));
    f.common->capabilities.samplerReduction = false;
    CHECK_VALIDATION(f, "min/max reduction is not supported",
                     f.device->writeSampler(0, {.reductionMode = ReductionMode::Maximum}));
}

TEST_CASE("descriptor slot allocator")
{
    ValidationFixture f;
    CHECK_VALIDATION(f, "are outside the",
                     DescriptorAllocator slots(f.device, {.firstSlot = 1, .slotCount = f.common->resourceHeapSize}));
    DescriptorAllocator slots(f.device, {.firstSlot = 1, .slotCount = 8});
    Semaphore* semaphore = f.device->createSemaphore({}).value();
    CHECK_VALIDATION(f, "was not handed out by this allocator", slots.release(5, {semaphore, 1}));
    uint32_t slot = slots.allocate();
    CHECK_VALIDATION(f, "the semaphore is null", slots.release(slot, {}));
    f.device->destroy(semaphore);
    ValidationFixture uninitialized(false);
    CHECK_VALIDATIONS(uninitialized, 2, "the device is not initialized",
                      DescriptorAllocator unusable(uninitialized.device, {.slotCount = 1}));
}
