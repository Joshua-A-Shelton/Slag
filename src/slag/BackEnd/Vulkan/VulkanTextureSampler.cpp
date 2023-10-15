#include "VulkanTextureSampler.h"
#include "VulkanLib.h"

namespace slag
{
    namespace vulkan
    {
        VkFilter VulkanTextureSampler::filterFromCrossPlatform(TextureSampler::Filter filter)
        {
            switch (filter)
            {
#define DEFINITION(slagName, vulkanName, dx12Name) case slagName: return vulkanName;
                SAMPLER_FILTER_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
        }

        VkSamplerAddressMode VulkanTextureSampler::addressModeFromCrossPlatform(TextureSampler::AddressMode mode)
        {
            switch (mode)
            {
#define DEFINITION(slagName, vulkanName, dx12Name) case slagName: return vulkanName;
                SAMPLER_ADDRESS_MODES_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
        }

        void *VulkanTextureSampler::GPUID()
        {
            return _sampler;
        }

        VulkanTextureSampler::VulkanTextureSampler(TextureSampler::Filter minFilter, TextureSampler::Filter magFilter, TextureSampler::AddressMode u, TextureSampler::AddressMode v,
                                                   TextureSampler::AddressMode w, bool destroyImmediate)
        {
            destroyImmediately = destroyImmediate;
            _min = minFilter;
            _mag = magFilter;
            _u = u;
            _v = v;
            _w = w;

            VkSamplerCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            createInfo.magFilter = filterFromCrossPlatform(minFilter);
            createInfo.minFilter = filterFromCrossPlatform(magFilter);
            createInfo.addressModeU = addressModeFromCrossPlatform(u);
            createInfo.addressModeV = addressModeFromCrossPlatform(v);
            createInfo.addressModeW = addressModeFromCrossPlatform(w);
            vkCreateSampler(VulkanLib::graphicsCard()->device(),&createInfo, nullptr,&_sampler);

            auto sampler = _sampler;
            freeResources = [=]()
            {
                if(_sampler)
                {
                    vkDestroySampler(VulkanLib::graphicsCard()->device(),sampler, nullptr);
                }

            };
        }

        VulkanTextureSampler::~VulkanTextureSampler()
        {
            if(_sampler)
            {
                smartDestroy();
            }
        }

        TextureSampler::Filter VulkanTextureSampler::magFilter()
        {
            return _mag;
        }

        TextureSampler::Filter VulkanTextureSampler::minFilter()
        {
            return _min;
        }

        TextureSampler::AddressMode VulkanTextureSampler::addressU()
        {
            return _u;
        }

        TextureSampler::AddressMode VulkanTextureSampler::addressV()
        {
            return _v;
        }

        TextureSampler::AddressMode VulkanTextureSampler::addressW()
        {
            return _w;
        }

        VkSampler VulkanTextureSampler::vulkanSampler()
        {
            return _sampler;
        }

        VulkanTextureSampler::VulkanTextureSampler(VulkanTextureSampler&& from):Resource(std::move(from))
        {
            move(std::move(from));
        }

        VulkanTextureSampler& VulkanTextureSampler::operator=(VulkanTextureSampler&& from)
        {
            Resource::operator=(std::move(from));
            move(std::move(from));
            return *this;
        }

        void VulkanTextureSampler::move(VulkanTextureSampler&& from)
        {
            std::swap(_sampler,from._sampler);
            std::swap(_min,from._min);
            std::swap(_mag,from._mag);
            std::swap(_u,from._u);
            std::swap(_v,from._v);
            std::swap(_w,from._w);
        }
    } // slag
} // vulkan