#pragma once

// AttachmentView and AttachmentViewDesc (ADR 0035).

#include <slag/PixelFormat.h>

#include <cstdint>

namespace slag {

class Texture;

// A render-target or depth/stencil view of one mip of a texture: the only
// view object in Slag (ADR 0004). The view type follows from the texture's
// type and the layer count. Read-only depth/stencil is chosen when rendering
// begins, not here.
struct AttachmentViewDesc
{
    Texture* texture = nullptr;                  // created with RenderTarget or DepthStencil
    PixelFormat format = PixelFormat::Undefined; // Undefined: the texture's format; else one of its castable formats
    uint32_t mip = 0;
    uint32_t baseLayer = 0; // depth slice for 3D textures, face for cubes (6 * cube + face)
    uint32_t layerCount = 1;
    const char* debugName = nullptr;
    uint64_t userData = 0;
};

class AttachmentView
{
public:
    uint64_t userData() const;
    void setUserData(uint64_t value);

    AttachmentView(const AttachmentView&) = delete;
    AttachmentView& operator=(const AttachmentView&) = delete;

protected:
    AttachmentView() = default;
    ~AttachmentView() = default;
};

} // namespace slag
