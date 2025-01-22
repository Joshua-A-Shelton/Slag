#include "GpuMemoryBarriers.h"
#include <unordered_set>
namespace slag
{
    BarrierAccess BarrierAccess::operator| (BarrierAccess b) const
    {
        return BarrierAccess(_value | b._value);
    }

    BarrierAccess& BarrierAccess::operator |=(BarrierAccess b)
    {
        _value = _value|b._value;
        return *this;
    }

    BarrierAccess BarrierAccess::operator&(BarrierAccess b) const
    {
        return BarrierAccess(_value & b._value);
    }

    BarrierAccess& BarrierAccess::operator&=(BarrierAccess b)
    {
        _value = _value&b._value;
        return *this;
    }

    BarrierAccess BarrierAccess::operator~() const
    {
        return BarrierAccess(~_value);
    }

    bool BarrierAccess::operator==(BarrierAccess b)const
    {
        return _value==b._value;
    }

    bool BarrierAccess::operator!=(BarrierAccess b)const
    {
        return _value!=b._value;
    }

    BarrierAccess BarrierAccess::compatibleAccess(Texture::Layout layout)
    {
        switch (layout)
        {

            case Texture::UNDEFINED:
                return BarrierAccessFlags::NONE;
            case Texture::RENDER_TARGET:
                return
                    //BarrierAccessFlags::NONE |
                    //BarrierAccessFlags::INDIRECT_COMMAND |
                    //BarrierAccessFlags::INDEX |
                    //BarrierAccessFlags::VERTEX_ATTRIBUTE |
                    //BarrierAccessFlags::UNIFORM |
                    //BarrierAccessFlags::INPUT_ATTACHMENT |
                    //BarrierAccessFlags::SHADER_READ |
                    //BarrierAccessFlags::SHADER_WRITE |
                        BarrierAccessFlags::COLOR_ATTACHMENT_READ |
                        BarrierAccessFlags::COLOR_ATTACHMENT_WRITE |
                        BarrierAccessFlags::DEPTH_STENCIL_READ |
                        BarrierAccessFlags::DEPTH_STENCIL_WRITE;
                //BarrierAccessFlags::TRANSFER_READ |
                //BarrierAccessFlags::TRANSFER_WRITE;
            case Texture::GENERAL:
                return
                    //BarrierAccessFlags::NONE |
                    //BarrierAccessFlags::INDIRECT_COMMAND |
                    //BarrierAccessFlags::INDEX |
                    //BarrierAccessFlags::VERTEX_ATTRIBUTE |
                    //BarrierAccessFlags::UNIFORM |
                        BarrierAccessFlags::INPUT_ATTACHMENT |
                        //BarrierAccessFlags::SHADER_READ |
                        //BarrierAccessFlags::SHADER_WRITE |
                        //BarrierAccessFlags::COLOR_ATTACHMENT_READ |
                        //BarrierAccessFlags::COLOR_ATTACHMENT_WRITE |
                        //BarrierAccessFlags::DEPTH_STENCIL_READ |
                        //BarrierAccessFlags::DEPTH_STENCIL_WRITE |
                        BarrierAccessFlags::TRANSFER_READ |
                        BarrierAccessFlags::TRANSFER_WRITE;
            case Texture::UNORDERED:
                return
                    //BarrierAccessFlags::NONE |
                    //BarrierAccessFlags::INDIRECT_COMMAND |
                    //BarrierAccessFlags::INDEX |
                    //BarrierAccessFlags::VERTEX_ATTRIBUTE |
                    //BarrierAccessFlags::UNIFORM |
                    //BarrierAccessFlags::INPUT_ATTACHMENT |
                        BarrierAccessFlags::SHADER_READ |
                        BarrierAccessFlags::SHADER_WRITE;
                //BarrierAccessFlags::COLOR_ATTACHMENT_READ |
                //BarrierAccessFlags::COLOR_ATTACHMENT_WRITE
                //BarrierAccessFlags::DEPTH_STENCIL_READ |
                //BarrierAccessFlags::DEPTH_STENCIL_WRITE;
                //BarrierAccessFlags::TRANSFER_READ |
                //BarrierAccessFlags::TRANSFER_WRITE;
            case Texture::DEPTH_TARGET_READ_ONLY:
                return
                    //BarrierAccessFlags::NONE |
                    //BarrierAccessFlags::INDIRECT_COMMAND |
                    //BarrierAccessFlags::INDEX |
                    //BarrierAccessFlags::VERTEX_ATTRIBUTE |
                    //BarrierAccessFlags::UNIFORM |
                    //BarrierAccessFlags::INPUT_ATTACHMENT |
                    //BarrierAccessFlags::SHADER_READ |
                    //BarrierAccessFlags::SHADER_WRITE |
                    //BarrierAccessFlags::COLOR_ATTACHMENT_READ |
                    //BarrierAccessFlags::COLOR_ATTACHMENT_WRITE |
                        BarrierAccessFlags::DEPTH_STENCIL_READ;
                //BarrierAccessFlags::DEPTH_STENCIL_WRITE;
                //BarrierAccessFlags::TRANSFER_READ |
                //BarrierAccessFlags::TRANSFER_WRITE;
            case Texture::DEPTH_TARGET:
                return
                    //BarrierAccessFlags::NONE |
                    //BarrierAccessFlags::INDIRECT_COMMAND |
                    //BarrierAccessFlags::INDEX |
                    //BarrierAccessFlags::VERTEX_ATTRIBUTE |
                    //BarrierAccessFlags::UNIFORM |
                    //BarrierAccessFlags::INPUT_ATTACHMENT |
                    //BarrierAccessFlags::SHADER_READ |
                    //BarrierAccessFlags::SHADER_WRITE |
                    //BarrierAccessFlags::COLOR_ATTACHMENT_READ |
                    //BarrierAccessFlags::COLOR_ATTACHMENT_WRITE |
                    //BarrierAccessFlags::DEPTH_STENCIL_READ |
                        BarrierAccessFlags::DEPTH_STENCIL_WRITE;
                //BarrierAccessFlags::TRANSFER_READ |
                //BarrierAccessFlags::TRANSFER_WRITE;
            case Texture::SHADER_RESOURCE:
                return
                    //BarrierAccessFlags::NONE |
                    //BarrierAccessFlags::INDIRECT_COMMAND |
                    //BarrierAccessFlags::INDEX |
                    //BarrierAccessFlags::VERTEX_ATTRIBUTE |
                    //BarrierAccessFlags::UNIFORM |
                        BarrierAccessFlags::INPUT_ATTACHMENT;
                //BarrierAccessFlags::SHADER_READ |
                //BarrierAccessFlags::SHADER_WRITE;
                //BarrierAccessFlags::COLOR_ATTACHMENT_READ |
                //BarrierAccessFlags::COLOR_ATTACHMENT_WRITE;
                //BarrierAccessFlags::DEPTH_STENCIL_READ |
                //BarrierAccessFlags::DEPTH_STENCIL_WRITE;
                //BarrierAccessFlags::TRANSFER_READ |
                //BarrierAccessFlags::TRANSFER_WRITE;
            case Texture::TRANSFER_DESTINATION:
                return
                    //BarrierAccessFlags::NONE |
                    //BarrierAccessFlags::INDIRECT_COMMAND |
                    //BarrierAccessFlags::INDEX |
                    //BarrierAccessFlags::VERTEX_ATTRIBUTE |
                    //BarrierAccessFlags::UNIFORM |
                    //BarrierAccessFlags::INPUT_ATTACHMENT |
                    //BarrierAccessFlags::SHADER_READ |
                    //BarrierAccessFlags::SHADER_WRITE |
                    //BarrierAccessFlags::COLOR_ATTACHMENT_READ |
                    //BarrierAccessFlags::COLOR_ATTACHMENT_WRITE |
                    //BarrierAccessFlags::DEPTH_STENCIL_READ |
                    //BarrierAccessFlags::DEPTH_STENCIL_WRITE |
                    //BarrierAccessFlags::TRANSFER_READ |
                        BarrierAccessFlags::TRANSFER_WRITE;
            case Texture::TRANSFER_SOURCE:
                return
                    //BarrierAccessFlags::NONE |
                    //BarrierAccessFlags::INDIRECT_COMMAND |
                    //BarrierAccessFlags::INDEX |
                    //BarrierAccessFlags::VERTEX_ATTRIBUTE |
                    //BarrierAccessFlags::UNIFORM |
                    //BarrierAccessFlags::INPUT_ATTACHMENT |
                    //BarrierAccessFlags::SHADER_READ |
                    //BarrierAccessFlags::SHADER_WRITE |
                    //BarrierAccessFlags::COLOR_ATTACHMENT_READ |
                    //BarrierAccessFlags::COLOR_ATTACHMENT_WRITE |
                    //BarrierAccessFlags::DEPTH_STENCIL_READ |
                    //BarrierAccessFlags::DEPTH_STENCIL_WRITE |
                        BarrierAccessFlags::TRANSFER_READ;
                //BarrierAccessFlags::TRANSFER_WRITE;
            case Texture::RESOLVE_DESTINATION:
                return
                    //BarrierAccessFlags::NONE |
                    //BarrierAccessFlags::INDIRECT_COMMAND |
                    //BarrierAccessFlags::INDEX |
                    //BarrierAccessFlags::VERTEX_ATTRIBUTE |
                    //BarrierAccessFlags::UNIFORM |
                    //BarrierAccessFlags::INPUT_ATTACHMENT |
                    //BarrierAccessFlags::SHADER_READ |
                    //BarrierAccessFlags::SHADER_WRITE |
                    //BarrierAccessFlags::COLOR_ATTACHMENT_READ |
                    //BarrierAccessFlags::COLOR_ATTACHMENT_WRITE |
                    //BarrierAccessFlags::DEPTH_STENCIL_READ |
                    //BarrierAccessFlags::DEPTH_STENCIL_WRITE |
                    //BarrierAccessFlags::TRANSFER_READ |
                        BarrierAccessFlags::TRANSFER_WRITE;
            case Texture::RESOLVE_SOURCE:
                return
                    //BarrierAccessFlags::NONE |
                    //BarrierAccessFlags::INDIRECT_COMMAND |
                    //BarrierAccessFlags::INDEX |
                    //BarrierAccessFlags::VERTEX_ATTRIBUTE |
                    //BarrierAccessFlags::UNIFORM |
                    //BarrierAccessFlags::INPUT_ATTACHMENT |
                    //BarrierAccessFlags::SHADER_READ |
                    //BarrierAccessFlags::SHADER_WRITE |
                    //BarrierAccessFlags::COLOR_ATTACHMENT_READ |
                    //BarrierAccessFlags::COLOR_ATTACHMENT_WRITE |
                    //BarrierAccessFlags::DEPTH_STENCIL_READ |
                    //BarrierAccessFlags::DEPTH_STENCIL_WRITE |
                        BarrierAccessFlags::TRANSFER_READ;
                //BarrierAccessFlags::TRANSFER_WRITE;
            case Texture::PRESENT:
                return
                    //BarrierAccessFlags::NONE |
                    //BarrierAccessFlags::INDIRECT_COMMAND |
                    //BarrierAccessFlags::INDEX |
                    //BarrierAccessFlags::VERTEX_ATTRIBUTE |
                    //BarrierAccessFlags::UNIFORM |
                        BarrierAccessFlags::INPUT_ATTACHMENT |
                        //BarrierAccessFlags::SHADER_READ |
                        //BarrierAccessFlags::SHADER_WRITE |
                        //BarrierAccessFlags::COLOR_ATTACHMENT_READ |
                        //BarrierAccessFlags::COLOR_ATTACHMENT_WRITE |
                        //BarrierAccessFlags::DEPTH_STENCIL_READ |
                        //BarrierAccessFlags::DEPTH_STENCIL_WRITE |
                        BarrierAccessFlags::TRANSFER_READ |
                        BarrierAccessFlags::TRANSFER_WRITE;
        }
        return BarrierAccessFlags::NONE;
    }

    BarrierAccess BarrierAccess::compatibleAccess(Texture* texture)
    {
        auto usage = texture->usage();
        BarrierAccess access = BarrierAccessFlags::TRANSFER_WRITE|BarrierAccessFlags::TRANSFER_READ;
        bool hasAccess = false;
        if(usage & TextureUsageFlags::SAMPLED_IMAGE || usage == TextureUsageFlags::SAMPLED_IMAGE)
        {
            hasAccess = true;
        }
        if(usage & TextureUsageFlags::INPUT_ATTACHMENT || usage == TextureUsageFlags::INPUT_ATTACHMENT)
        {
            access|=BarrierAccessFlags::INPUT_ATTACHMENT;
            hasAccess = true;
        }
        if(usage & TextureUsageFlags::STORAGE || usage == TextureUsageFlags::STORAGE)
        {
            access|=BarrierAccessFlags::SHADER_READ|BarrierAccessFlags::SHADER_WRITE;
            hasAccess = true;
        }
        if(usage & TextureUsageFlags::RENDER_TARGET_ATTACHMENT || usage == TextureUsageFlags::RENDER_TARGET_ATTACHMENT)
        {
            access|=BarrierAccessFlags::COLOR_ATTACHMENT_READ|BarrierAccessFlags::COLOR_ATTACHMENT_WRITE;
            hasAccess = true;
        }
        if(usage & TextureUsageFlags::DEPTH_STENCIL_ATTACHMENT || usage == TextureUsageFlags::DEPTH_STENCIL_ATTACHMENT)
        {
            access|=BarrierAccessFlags::DEPTH_STENCIL_READ|BarrierAccessFlags::DEPTH_STENCIL_WRITE;
            hasAccess = true;
        }

        if(!hasAccess)
        {
            return BarrierAccessFlags::NONE;
        }
        return access;
    }

    BarrierAccess BarrierAccess::compatibleAccess(GpuQueue::QueueType queueType)
    {
        //supported on everything...
        auto access = BarrierAccessFlags::TRANSFER_READ | BarrierAccessFlags::TRANSFER_WRITE;
        switch (queueType)
        {
            case GpuQueue::GRAPHICS:
                access |=
                        BarrierAccessFlags::INDIRECT_COMMAND |
                        BarrierAccessFlags::INDEX |
                        BarrierAccessFlags::VERTEX_ATTRIBUTE |
                        BarrierAccessFlags::UNIFORM |
                        BarrierAccessFlags::INPUT_ATTACHMENT |
                        BarrierAccessFlags::SHADER_READ |
                        BarrierAccessFlags::SHADER_WRITE |
                        BarrierAccessFlags::COLOR_ATTACHMENT_READ |
                        BarrierAccessFlags::COLOR_ATTACHMENT_WRITE |
                        BarrierAccessFlags::DEPTH_STENCIL_READ |
                        BarrierAccessFlags::DEPTH_STENCIL_WRITE |
                        BarrierAccessFlags::TRANSFER_READ |
                        BarrierAccessFlags::TRANSFER_WRITE;
                break;
            case GpuQueue::COMPUTE:
                access |=
                        BarrierAccessFlags::INDIRECT_COMMAND |
                        //BarrierAccessFlags::INDEX |
                        //BarrierAccessFlags::VERTEX_ATTRIBUTE |
                        BarrierAccessFlags::UNIFORM |
                        BarrierAccessFlags::INPUT_ATTACHMENT |
                        BarrierAccessFlags::SHADER_READ |
                        BarrierAccessFlags::SHADER_WRITE |
                        //BarrierAccessFlags::COLOR_ATTACHMENT_READ |
                        //BarrierAccessFlags::COLOR_ATTACHMENT_WRITE |
                        //BarrierAccessFlags::DEPTH_STENCIL_READ |
                        //BarrierAccessFlags::DEPTH_STENCIL_WRITE |
                        BarrierAccessFlags::TRANSFER_READ |
                        BarrierAccessFlags::TRANSFER_WRITE;
                break;
            case GpuQueue::TRANSFER:
                access |=
                        BarrierAccessFlags::INDIRECT_COMMAND |
                        //BarrierAccessFlags::INDEX |
                        //BarrierAccessFlags::VERTEX_ATTRIBUTE |
                        //BarrierAccessFlags::UNIFORM |
                        //BarrierAccessFlags::INPUT_ATTACHMENT |
                        //BarrierAccessFlags::SHADER_READ |
                        //BarrierAccessFlags::SHADER_WRITE |
                        //BarrierAccessFlags::COLOR_ATTACHMENT_READ |
                        //BarrierAccessFlags::COLOR_ATTACHMENT_WRITE |
                        //BarrierAccessFlags::DEPTH_STENCIL_READ |
                        //BarrierAccessFlags::DEPTH_STENCIL_WRITE |
                        BarrierAccessFlags::TRANSFER_READ |
                        BarrierAccessFlags::TRANSFER_WRITE;
                break;
        }
        return access;
    }

    BarrierAccess BarrierAccess::compatibleAccess(Texture* texture, Texture::Layout toLayout, GpuQueue::QueueType queueType)
    {
        auto accessTexture = compatibleAccess(texture);
        if(accessTexture == BarrierAccessFlags::NONE)
        {
            return accessTexture;
        }
        auto accessLayout = compatibleAccess(toLayout);
        if(accessLayout == BarrierAccessFlags::NONE)
        {
            return accessLayout;
        }
        auto accessQueue = compatibleAccess(queueType);
        return accessTexture & accessLayout & accessQueue;
    }

    std::vector<Texture::Layout> BarrierUtils::compatibleLayouts(GpuQueue::QueueType queueType)
    {
        switch (queueType)
        {
            case GpuQueue::GRAPHICS:
                return std::vector<Texture::Layout>
                        {
                                Texture::UNDEFINED,
                                Texture::RENDER_TARGET,
                                Texture::GENERAL,
                                Texture::UNORDERED,
                                Texture::DEPTH_TARGET_READ_ONLY,
                                Texture::DEPTH_TARGET,
                                Texture::SHADER_RESOURCE,
                                Texture::TRANSFER_DESTINATION,
                                Texture::TRANSFER_SOURCE,
                                Texture::RESOLVE_DESTINATION,
                                Texture::RESOLVE_SOURCE,
                                Texture::PRESENT
                        };

            case GpuQueue::COMPUTE:
                return std::vector<Texture::Layout>
                        {
                                Texture::UNDEFINED,
                                //Texture::RENDER_TARGET,
                                Texture::GENERAL,
                                Texture::UNORDERED,
                                //Texture::DEPTH_TARGET_READ_ONLY,
                                //Texture::DEPTH_TARGET,
                                Texture::SHADER_RESOURCE,
                                Texture::TRANSFER_DESTINATION,
                                Texture::TRANSFER_SOURCE,
                                Texture::RESOLVE_DESTINATION,
                                Texture::RESOLVE_SOURCE,
                                Texture::PRESENT
                        };
            case GpuQueue::TRANSFER:
                return std::vector<Texture::Layout>
                        {
                                Texture::UNDEFINED,
                                //Texture::RENDER_TARGET,
                                Texture::GENERAL,
                                //Texture::UNORDERED,
                                //Texture::DEPTH_TARGET_READ_ONLY,
                                //Texture::DEPTH_TARGET,
                                //Texture::SHADER_RESOURCE,
                                //Texture::TRANSFER_DESTINATION,
                                //Texture::TRANSFER_SOURCE,
                                //Texture::RESOLVE_DESTINATION,
                                //Texture::RESOLVE_SOURCE,
                                Texture::PRESENT
                        };
        }
        return std::vector<Texture::Layout>();
    }

    std::vector<Texture::Layout> BarrierUtils::compatibleLayouts(Texture* texture)
    {
        std::unordered_set<Texture::Layout> layouts;
        auto usage = texture->usage();

        if(usage & TextureUsageFlags::SAMPLED_IMAGE || usage == TextureUsageFlags::SAMPLED_IMAGE)
        {
            layouts.insert(Texture::UNDEFINED);
            //layouts.insert(Texture::RENDER_TARGET);
            layouts.insert(Texture::GENERAL);
            layouts.insert(Texture::UNORDERED);
            //layouts.insert(Texture::DEPTH_TARGET_READ_ONLY);
            //layouts.insert(Texture::DEPTH_TARGET);
            layouts.insert(Texture::SHADER_RESOURCE);
            layouts.insert(Texture::TRANSFER_DESTINATION);
            layouts.insert(Texture::TRANSFER_SOURCE);
            layouts.insert(Texture::RESOLVE_DESTINATION);
            layouts.insert(Texture::RESOLVE_SOURCE);
            layouts.insert(Texture::PRESENT);
        }
        if(usage & TextureUsageFlags::INPUT_ATTACHMENT || usage == TextureUsageFlags::INPUT_ATTACHMENT)
        {
            layouts.insert(Texture::UNDEFINED);
            //layouts.insert(Texture::RENDER_TARGET);
            layouts.insert(Texture::GENERAL);
            layouts.insert(Texture::UNORDERED);
            //layouts.insert(Texture::DEPTH_TARGET_READ_ONLY);
            //layouts.insert(Texture::DEPTH_TARGET);
            layouts.insert(Texture::SHADER_RESOURCE);
            layouts.insert(Texture::TRANSFER_DESTINATION);
            layouts.insert(Texture::TRANSFER_SOURCE);
            layouts.insert(Texture::RESOLVE_DESTINATION);
            layouts.insert(Texture::RESOLVE_SOURCE);
            layouts.insert(Texture::PRESENT);
        }
        if(usage & TextureUsageFlags::STORAGE || usage == TextureUsageFlags::STORAGE)
        {
            layouts.insert(Texture::UNDEFINED);
            //layouts.insert(Texture::RENDER_TARGET);
            layouts.insert(Texture::GENERAL);
            layouts.insert(Texture::UNORDERED);
            //layouts.insert(Texture::DEPTH_TARGET_READ_ONLY);
            //layouts.insert(Texture::DEPTH_TARGET);
            //layouts.insert(Texture::SHADER_RESOURCE);
            layouts.insert(Texture::TRANSFER_DESTINATION);
            layouts.insert(Texture::TRANSFER_SOURCE);
            layouts.insert(Texture::RESOLVE_DESTINATION);
            layouts.insert(Texture::RESOLVE_SOURCE);
            layouts.insert(Texture::PRESENT);
        }
        if(usage & TextureUsageFlags::RENDER_TARGET_ATTACHMENT || usage == TextureUsageFlags::RENDER_TARGET_ATTACHMENT)
        {
            layouts.insert(Texture::UNDEFINED);
            layouts.insert(Texture::RENDER_TARGET);
            layouts.insert(Texture::GENERAL);
            layouts.insert(Texture::UNORDERED);
            //layouts.insert(Texture::DEPTH_TARGET_READ_ONLY);
            //layouts.insert(Texture::DEPTH_TARGET);
            //layouts.insert(Texture::SHADER_RESOURCE);
            layouts.insert(Texture::TRANSFER_DESTINATION);
            layouts.insert(Texture::TRANSFER_SOURCE);
            layouts.insert(Texture::RESOLVE_DESTINATION);
            layouts.insert(Texture::RESOLVE_SOURCE);
            layouts.insert(Texture::PRESENT);
        }
        if(usage & TextureUsageFlags::DEPTH_STENCIL_ATTACHMENT || usage == TextureUsageFlags::DEPTH_STENCIL_ATTACHMENT)
        {
            layouts.insert(Texture::UNDEFINED);
            //layouts.insert(Texture::RENDER_TARGET);
            layouts.insert(Texture::GENERAL);
            layouts.insert(Texture::UNORDERED);
            layouts.insert(Texture::DEPTH_TARGET_READ_ONLY);
            layouts.insert(Texture::DEPTH_TARGET);
            //layouts.insert(Texture::SHADER_RESOURCE);
            layouts.insert(Texture::TRANSFER_DESTINATION);
            layouts.insert(Texture::TRANSFER_SOURCE);
            layouts.insert(Texture::RESOLVE_DESTINATION);
            layouts.insert(Texture::RESOLVE_SOURCE);
            layouts.insert(Texture::PRESENT);
        }
        std::vector<Texture::Layout> texLayouts;
        for(auto& layout: layouts)
        {
            texLayouts.push_back(layout);
        }
        return texLayouts;

    }

    std::vector<Texture::Layout> BarrierUtils::compatibleLayouts(GpuQueue::QueueType queueType, Texture* texture)
    {
        std::vector<Texture::Layout> unionLayouts;
        auto queueLayouts = compatibleLayouts(queueType);
        auto textureLayouts = compatibleLayouts(texture);
        for(auto i=0; i< queueLayouts.size(); i++)
        {
            auto queueLayout = queueLayouts[i];
            for(auto j=0; j< textureLayouts.size(); j++)
            {
                if(textureLayouts[j]==queueLayout)
                {
                    unionLayouts.push_back(queueLayout);
                    break;
                }
            }
        }
        return unionLayouts;
    }
}