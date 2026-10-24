#include "common/Logging.h"
#include "common/Objects.h"

#include <slag/AttachmentView.h>
#include <slag/GraphicsDevice.h>

namespace slag {

namespace {

[[maybe_unused]] bool castableTo(const TextureCommon& texture, PixelFormat format)
{
    for (uint32_t i = 0; i < texture.castableCount; ++i)
    {
        if (texture.castableFormats[i] == format)
        {
            return true;
        }
    }
    return false;
}

} // namespace

Result<AttachmentView*> GraphicsDevice::createAttachmentView(const AttachmentViewDesc& desc)
{
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    auto* texture = static_cast<TextureCommon*>(desc.texture);
    [[maybe_unused]] InstanceCommon* instance = self->instance;
    SLAG_VALIDATE(instance, self->initialized, "GraphicsDevice::createAttachmentView",
                  "device \"%s\" is not initialized", self->info.name);
    SLAG_VALIDATE(instance, texture, "GraphicsDevice::createAttachmentView", "texture is null");
    if (!texture)
    {
        return Status::Unsupported;
    }
    AttachmentViewDesc resolved = desc;
    if (resolved.format == PixelFormat::Undefined)
    {
        resolved.format = texture->format;
    }
#ifdef SLAG_VALIDATION
    SLAG_VALIDATE(instance, texture->device == self, "GraphicsDevice::createAttachmentView",
                  "the texture belongs to another device");
    SLAG_VALIDATE(
        instance, (texture->usage & (TextureUsage::RenderTarget | TextureUsage::DepthStencil)) != TextureUsage::None,
        "GraphicsDevice::createAttachmentView", "the texture has neither RenderTarget nor DepthStencil usage");
    SLAG_VALIDATE(instance, resolved.format == texture->format || castableTo(*texture, resolved.format),
                  "GraphicsDevice::createAttachmentView", "%s is neither the texture's format nor castable",
                  formatInfo(resolved.format).name);
    SLAG_VALIDATE(instance, desc.mip < texture->mipCount, "GraphicsDevice::createAttachmentView",
                  "mip %u is outside the texture (%u mips)", desc.mip, texture->mipCount);
    // 3D textures: a range of depth slices of that mip.
    uint32_t layers = texture->layerCount;
    if (texture->type == TextureType::Texture3D)
    {
        layers = texture->depth >> desc.mip ? texture->depth >> desc.mip : 1;
    }
    SLAG_VALIDATE(instance,
                  desc.layerCount >= 1 && desc.baseLayer < layers && desc.layerCount <= layers - desc.baseLayer,
                  "GraphicsDevice::createAttachmentView", "layers %u..%u are outside the texture (%u)", desc.baseLayer,
                  desc.baseLayer + desc.layerCount, layers);
#endif
    if (self->lost)
    {
        return Status::DeviceLost;
    }
    Result<AttachmentViewCommon*> view = SLAG_DISPATCH(self, createAttachmentView)(self, resolved);
    if (!view)
    {
        return view.status();
    }
    AttachmentViewCommon* object = view.value();
    object->userData = desc.userData;
    object->texture = texture;
    object->format = resolved.format;
    object->mip = desc.mip;
    object->baseLayer = desc.baseLayer;
    object->layerCount = desc.layerCount;
    ++texture->liveViews;
    return static_cast<AttachmentView*>(object);
}

void GraphicsDevice::destroy(AttachmentView* view)
{
    if (!view)
    {
        return;
    }
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    auto* object = static_cast<AttachmentViewCommon*>(view);
    SLAG_VALIDATE(self->instance, object->device == self, "GraphicsDevice::destroy",
                  "the attachment view belongs to another device");
    TextureCommon* texture = object->texture;
    SLAG_DISPATCH(self, destroyAttachmentView)(self, object);
    --texture->liveViews;
}

uint64_t AttachmentView::userData() const
{
    return static_cast<const AttachmentViewCommon*>(this)->userData;
}

void AttachmentView::setUserData(uint64_t value)
{
    static_cast<AttachmentViewCommon*>(this)->userData = value;
}

} // namespace slag
