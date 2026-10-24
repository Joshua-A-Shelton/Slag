#include "d3d12/D3D12Backend.h"
#include "d3d12/D3D12Formats.h"

#include <cstring>

// Poison descriptors (ADRs 0016, 0031), validation builds: a cleared slot, or
// one whose resource was destroyed, gets a view of the same kind onto a small
// resource full of poison values, so bad reads show up instead of D3D12's
// silent zeros: magenta for float textures, 0xDEADBEEF for everything else.

namespace slag::d3d12 {

#ifdef SLAG_VALIDATION

namespace {

constexpr UINT64 poisonBufferSize = 65536;
constexpr uint32_t poisonWord = 0xDEADBEEF;
constexpr UINT poisonSize = 4; // texels per side

enum FormatClass
{
    FloatClass,
    UintClass,
    SintClass,
};

enum Shape
{
    Shape1D,
    Shape2DArray, // 6 layers: 2D, 2D array, cube, cube array views
    Shape3D,
};

const DXGI_FORMAT classFormats[3] = {DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_UINT,
                                     DXGI_FORMAT_R32G32B32A32_SINT};

FormatClass formatClass(PixelFormat format)
{
    if (!hasTraits(format, FormatTraits::Integer))
    {
        return FloatClass;
    }
    return std::strstr(formatInfo(format).name, "Sint") ? SintClass : UintClass;
}

D3D12_RESOURCE_DESC1 textureDesc(Shape shape, DXGI_FORMAT format, bool writable)
{
    D3D12_RESOURCE_DESC1 desc = {};
    desc.Dimension = shape == Shape1D   ? D3D12_RESOURCE_DIMENSION_TEXTURE1D
                     : shape == Shape3D ? D3D12_RESOURCE_DIMENSION_TEXTURE3D
                                        : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = poisonSize;
    desc.Height = shape == Shape1D ? 1 : poisonSize;
    desc.DepthOrArraySize = static_cast<UINT16>(shape == Shape2DArray ? 6 : shape == Shape3D ? poisonSize : 1);
    desc.MipLevels = 1;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.Flags = writable ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
    return desc;
}

ID3D12Resource* committed(GraphicsDeviceImpl& device, D3D12_HEAP_TYPE type, const D3D12_RESOURCE_DESC1& desc,
                          D3D12_BARRIER_LAYOUT layout)
{
    D3D12_HEAP_PROPERTIES heap = {};
    heap.Type = type;
    ID3D12Resource* resource = nullptr;
    if (FAILED(device.device->CreateCommittedResource3(&heap, D3D12_HEAP_FLAG_NONE, &desc, layout, nullptr, nullptr, 0,
                                                       nullptr, IID_PPV_ARGS(&resource))))
    {
        return nullptr;
    }
    setObjectName(resource, "slag poison");
    return resource;
}

} // namespace

// Fills the poison resources with one copy on the first graphics queue. Best
// effort: without them, slots are left as they are.
void createPoison(GraphicsDeviceImpl& device)
{
    D3D12_RESOURCE_DESC1 bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = poisonBufferSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    D3D12_RESOURCE_DESC1 uploadDesc = bufferDesc;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    device.poisonBuffer = committed(device, D3D12_HEAP_TYPE_DEFAULT, bufferDesc, D3D12_BARRIER_LAYOUT_UNDEFINED);
    for (int formatClass = 0; formatClass < 3; ++formatClass)
    {
        for (int writable = 0; writable < 2; ++writable)
        {
            for (int shape = 0; shape < 3; ++shape)
            {
                device.poisonTextures[formatClass][writable][shape] =
                    committed(device, D3D12_HEAP_TYPE_DEFAULT,
                              textureDesc(static_cast<Shape>(shape), classFormats[formatClass], writable != 0),
                              D3D12_BARRIER_LAYOUT_COPY_DEST);
            }
        }
    }
    // One upload buffer: the poison buffer's words, then every texture subresource.
    ID3D12Resource* upload = committed(device, D3D12_HEAP_TYPE_UPLOAD, uploadDesc, D3D12_BARRIER_LAYOUT_UNDEFINED);
    ID3D12CommandAllocator* allocator = nullptr;
    ID3D12GraphicsCommandList7* list = nullptr;
    void* mapped = nullptr;
    if (!device.poisonBuffer || !upload || device.queues[0].empty() ||
        FAILED(device.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator))) ||
        FAILED(device.device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE,
                                                 IID_PPV_ARGS(&list))) ||
        FAILED(list->Reset(allocator, nullptr)) || FAILED(upload->Map(0, nullptr, &mapped)))
    {
        release(list);
        release(allocator);
        release(upload);
        return;
    }
    auto* words = static_cast<uint32_t*>(mapped);
    for (UINT64 i = 0; i < poisonBufferSize / 4; ++i)
    {
        words[i] = poisonWord;
    }
    // Texture rows come from the start of the upload buffer too: float texels
    // are magenta, integer ones 0xDEADBEEF. A 4-texel row is 64 bytes, a
    // subresource (pitch 256, 4 rows) 1 KB; 6 layers or 4 slices fit easily.
    constexpr UINT rowPitch = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
    constexpr UINT64 floatOffset = 32768;
    auto* floats = reinterpret_cast<float*>(static_cast<uint8_t*>(mapped) + floatOffset);
    for (UINT i = 0; i < 16384 / 4; i += 4)
    {
        floats[i] = 1.0f;
        floats[i + 1] = 0.0f;
        floats[i + 2] = 1.0f;
        floats[i + 3] = 1.0f;
    }
    upload->Unmap(0, nullptr);

    list->CopyBufferRegion(device.poisonBuffer, 0, upload, 0, poisonBufferSize);
    D3D12_TEXTURE_BARRIER barriers[18] = {};
    UINT barrierCount = 0;
    for (int formatClass = 0; formatClass < 3; ++formatClass)
    {
        for (int writable = 0; writable < 2; ++writable)
        {
            for (int shape = 0; shape < 3; ++shape)
            {
                ID3D12Resource* texture = device.poisonTextures[formatClass][writable][shape];
                if (!texture)
                {
                    continue;
                }
                UINT subresources = shape == Shape2DArray ? 6 : 1;
                UINT depth = shape == Shape3D ? poisonSize : 1;
                for (UINT subresource = 0; subresource < subresources; ++subresource)
                {
                    D3D12_TEXTURE_COPY_LOCATION destination = {};
                    destination.pResource = texture;
                    destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                    destination.SubresourceIndex = subresource;
                    D3D12_TEXTURE_COPY_LOCATION source = {};
                    source.pResource = upload;
                    source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                    source.PlacedFootprint.Offset = formatClass == FloatClass ? floatOffset : 0;
                    source.PlacedFootprint.Footprint = {classFormats[formatClass], poisonSize,
                                                        shape == Shape1D ? 1 : poisonSize, depth, rowPitch};
                    list->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
                }
                D3D12_TEXTURE_BARRIER& barrier = barriers[barrierCount++];
                barrier.SyncBefore = D3D12_BARRIER_SYNC_COPY;
                barrier.SyncAfter = D3D12_BARRIER_SYNC_NONE;
                barrier.AccessBefore = D3D12_BARRIER_ACCESS_COPY_DEST;
                barrier.AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
                barrier.LayoutBefore = D3D12_BARRIER_LAYOUT_COPY_DEST;
                barrier.LayoutAfter =
                    writable ? D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS : D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
                barrier.pResource = texture;
                barrier.Subresources.IndexOrFirstMipLevel = 0xFFFFFFFF;
            }
        }
    }
    D3D12_BARRIER_GROUP group = {D3D12_BARRIER_TYPE_TEXTURE, barrierCount};
    group.pTextureBarriers = barriers;
    list->Barrier(1, &group);
    list->Close();
    auto* queue = static_cast<CommandQueueImpl*>(device.queues[0][0]);
    ID3D12CommandList* lists[] = {list};
    queue->queue->ExecuteCommandLists(1, lists);
    (void)queueWaitIdle(queue);
    release(list);
    release(allocator);
    release(upload);
}

void destroyPoison(GraphicsDeviceImpl& device)
{
    release(device.poisonBuffer);
    for (auto& formatClass : device.poisonTextures)
    {
        for (auto& use : formatClass)
        {
            for (ID3D12Resource*& texture : use)
            {
                release(texture);
            }
        }
    }
}

#endif

void poisonResourceSlot([[maybe_unused]] GraphicsDeviceCommon* common, [[maybe_unused]] uint32_t index,
                        [[maybe_unused]] const SlotDescription& description)
{
#ifdef SLAG_VALIDATION
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    if (!device.poisonBuffer)
    {
        return;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE destination = {device.resourceHeap.cpuStart.ptr +
                                               SIZE_T(index) * device.resourceHeap.increment};
    if (description.content == SlotContent::Buffer)
    {
        BufferDescriptorKind kind = description.bufferKind;
        if (kind == BufferDescriptorKind::Constant)
        {
            D3D12_CONSTANT_BUFFER_VIEW_DESC view = {device.poisonBuffer->GetGPUVirtualAddress(), 256};
            device.device->CreateConstantBufferView(&view, destination);
            return;
        }
        bool writable = kind == BufferDescriptorKind::RWRaw || kind == BufferDescriptorKind::RWStructured ||
                        kind == BufferDescriptorKind::RWTyped;
        bool raw = kind == BufferDescriptorKind::Raw || kind == BufferDescriptorKind::RWRaw;
        bool structured = kind == BufferDescriptorKind::Structured || kind == BufferDescriptorKind::RWStructured;
        DXGI_FORMAT format = raw          ? DXGI_FORMAT_R32_TYPELESS
                             : structured ? DXGI_FORMAT_UNKNOWN
                                          : toDxgiFormat(description.format);
        UINT elementSize = raw ? 4 : structured ? description.stride : formatInfo(description.format).blockBytes;
        UINT elements = elementSize ? static_cast<UINT>(poisonBufferSize / elementSize) : 0;
        if (writable)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC view = {};
            view.Format = format;
            view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            view.Buffer.NumElements = elements;
            view.Buffer.StructureByteStride = structured ? description.stride : 0;
            view.Buffer.Flags = raw ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;
            device.device->CreateUnorderedAccessView(device.poisonBuffer, nullptr, &view, destination);
        }
        else
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
            view.Format = format;
            view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            view.Buffer.NumElements = elements;
            view.Buffer.StructureByteStride = structured ? description.stride : 0;
            view.Buffer.Flags = raw ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;
            device.device->CreateShaderResourceView(device.poisonBuffer, &view, destination);
        }
        return;
    }
    if (description.content != SlotContent::Texture)
    {
        return;
    }
    FormatClass formatClassIndex = formatClass(description.format);
    DXGI_FORMAT format = classFormats[formatClassIndex];
    TextureViewType type = description.viewType;
    Shape shape = type == TextureViewType::Texture1D || type == TextureViewType::Texture1DArray ? Shape1D
                  : type == TextureViewType::Texture3D                                          ? Shape3D
                                                                                                : Shape2DArray;
    ID3D12Resource* texture = device.poisonTextures[formatClassIndex][description.writable ? 1 : 0][shape];
    if (description.writable)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC view = {};
        view.Format = format;
        switch (type)
        {
        case TextureViewType::Texture1D:
            view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
            break;
        case TextureViewType::Texture1DArray:
            view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            view.Texture1DArray.ArraySize = 1;
            break;
        case TextureViewType::Texture2D:
            view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            break;
        case TextureViewType::Texture3D:
            view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            view.Texture3D.WSize = UINT(-1);
            break;
        default:
            view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            view.Texture2DArray.ArraySize = 6;
            break;
        }
        device.device->CreateUnorderedAccessView(texture, nullptr, &view, destination);
        return;
    }
    D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
    view.Format = format;
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    switch (type)
    {
    case TextureViewType::Texture1D:
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
        view.Texture1D.MipLevels = 1;
        break;
    case TextureViewType::Texture1DArray:
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        view.Texture1DArray = {0, 1, 0, 1, 0.0f};
        break;
    case TextureViewType::Texture2D:
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        view.Texture2D.MipLevels = 1;
        break;
    case TextureViewType::Texture2DArray:
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        view.Texture2DArray = {0, 1, 0, 6, 0, 0.0f};
        break;
    case TextureViewType::Texture3D:
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        view.Texture3D.MipLevels = 1;
        break;
    case TextureViewType::TextureCube:
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        view.TextureCube.MipLevels = 1;
        break;
    case TextureViewType::TextureCubeArray:
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        view.TextureCubeArray = {0, 1, 0, 1, 0.0f};
        break;
    case TextureViewType::Texture2DMS:
    case TextureViewType::Texture2DMSArray:
        // No multisampled poison: a null descriptor of the same dimension.
        view.ViewDimension = type == TextureViewType::Texture2DMS ? D3D12_SRV_DIMENSION_TEXTURE2DMS
                                                                  : D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
        view.Texture2DMSArray.ArraySize = 1;
        texture = nullptr;
        break;
    }
    device.device->CreateShaderResourceView(texture, &view, destination);
#endif
}

} // namespace slag::d3d12
