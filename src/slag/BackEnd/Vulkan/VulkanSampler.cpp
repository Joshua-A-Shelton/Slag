#include "VulkanSampler.h"
#include "VulkanLib.h"

namespace slag
{
    namespace vulkan
    {

        VulkanSampler::VulkanSampler(Sampler::Filter minFilter,
                                     Sampler::Filter magFilter,
                                     Sampler::Filter mipMapFilter,
                                     Sampler::AddressMode u,
                                     Sampler::AddressMode v,
                                     Sampler::AddressMode w,
                                     float mipLODBias,
                                     bool anisotrphyEnable,
                                     uint8_t maxAnisotrophy,
                                     Sampler::ComparisonFunction comparisonFunction,
                                     float minLOD,
                                     float maxLOD,
                                     Color borderColor,
                                     bool destroyImmediate):resources::Resource(destroyImmediate)
        {
            _min = minFilter;
            _mag = magFilter;
            _mip = mipMapFilter;
            _u = u;
            _v = v;
            _w = w;
            _mipLODBias = mipLODBias;
            _anisotrophyEnabled = anisotrphyEnable;
            _maxAniotrophy = maxAnisotrophy;
            _comparisionFunction = comparisonFunction;
            _minLOD = minLOD;
            _maxLOD = maxLOD;
            _borderColor = borderColor;

            VkClearColorValue borderColorVulkan;
            borderColorVulkan.float32[0] = _borderColor.red;
            borderColorVulkan.float32[1] = _borderColor.green;
            borderColorVulkan.float32[2] = _borderColor.blue;
            borderColorVulkan.float32[3] = _borderColor.alpha;

            VkSamplerCustomBorderColorCreateInfoEXT borderExt{};
            borderExt.sType = VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT;
            borderExt.customBorderColor = borderColorVulkan;
            borderExt.format = VK_FORMAT_R32G32B32A32_SFLOAT;

            VkSamplerCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            createInfo.magFilter = VulkanLib::filter(minFilter);
            createInfo.minFilter = VulkanLib::filter(magFilter);
            createInfo.mipmapMode = VulkanLib::mipMapMode(mipMapFilter);
            createInfo.addressModeU = VulkanLib::addressMode(u);
            createInfo.addressModeV = VulkanLib::addressMode(v);
            createInfo.addressModeW = VulkanLib::addressMode(w);
            createInfo.mipLodBias = _mipLODBias;
            createInfo.anisotropyEnable = _anisotrophyEnabled;
            createInfo.maxAnisotropy = _maxAniotrophy;
            createInfo.compareOp = VulkanLib::compareOp(comparisonFunction);
            createInfo.minLod = _minLOD;
            createInfo.maxLod = _maxLOD;
            createInfo.pNext = &borderExt;

            auto result =vkCreateSampler(VulkanLib::card()->device(),&createInfo, nullptr,&_sampler);

            auto sampler = _sampler;
            _disposeFunction = [=]()
            {
                if(sampler)
                {
                    vkDestroySampler(VulkanLib::card()->device(),sampler, nullptr);
                }

            };
            if(result!= VK_SUCCESS)
            {
                throw std::runtime_error("unable to create sampler");
            }
        }

        VulkanSampler::~VulkanSampler()
        {
            if(_sampler)
            {
                smartDestroy();
            }
        }

        void VulkanSampler::move(VulkanSampler&& from)
        {
            resources::Resource::move(from);
            std::swap(_sampler,from._sampler);
            std::swap(_min,from._min);
            std::swap(_mag,from._mag);
            std::swap(_u,from._u);
            std::swap(_v,from._v);
            std::swap(_w,from._w);
            std::swap(_mipLODBias,from._mipLODBias);
            std::swap(_anisotrophyEnabled,from._anisotrophyEnabled);
            std::swap(_maxAniotrophy,from._maxAniotrophy);
            std::swap(_comparisionFunction,from._comparisionFunction);
            std::swap(_minLOD,from._minLOD);
            std::swap(_minLOD,from._minLOD);
            std::swap(_borderColor,from._borderColor);
        }

        Sampler::Filter VulkanSampler::magFilter()
        {
            return _mag;
        }

        Sampler::Filter VulkanSampler::minFilter()
        {
            return _min;
        }

        Sampler::Filter VulkanSampler::mipMapFilter()
        {
            return _mip;
        }

        Sampler::AddressMode VulkanSampler::addressU()
        {
            return _u;
        }

        Sampler::AddressMode VulkanSampler::addressV()
        {
            return _v;
        }

        Sampler::AddressMode VulkanSampler::addressW()
        {
            return _w;
        }

        VkSampler VulkanSampler::vulkanSampler()
        {
            return _sampler;
        }

        VulkanSampler::VulkanSampler(VulkanSampler&& from):Resource(std::move(from))
        {
            move(std::move(from));
        }

        VulkanSampler& VulkanSampler::operator=(VulkanSampler&& from)
        {
            resources::Resource::move(from);
            move(std::move(from));
            return *this;
        }

        float VulkanSampler::mipLODBias()
        {
            return _mipLODBias;
        }

        bool VulkanSampler::anisotrophyEnabled()
        {
            return _anisotrophyEnabled;
        }

        uint8_t VulkanSampler::maxAnisotrophy()
        {
            return _maxAniotrophy;
        }

        Sampler::ComparisonFunction VulkanSampler::comparisonFunction()
        {
            return _comparisionFunction;
        }

        Color VulkanSampler::borderColor()
        {
            return _borderColor;
        }

        float VulkanSampler::minLOD()
        {
            return _minLOD;
        }

        float VulkanSampler::maxLOD()
        {
            return _maxLOD;
        }
    } // vulkan
} // slag