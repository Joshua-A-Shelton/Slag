#include "VulkanSampler.h"

#include "VulkanGraphicsCard.h"
#include "../VulkanBackend.h"

namespace slag
{
    namespace vulkan
    {
         VulkanSampler::VulkanSampler(SamplerParameters parameters)
        {
            _min = parameters.minFilter;
            _mag = parameters.magFilter;
            _mip = parameters.mipMapFilter;
            _u = parameters.u;
            _v = parameters.v;
            _w = parameters.w;
            _mipLODBias = parameters.mipLODBias;
            _anisotrophyEnabled = parameters.anisotrophyEnabled;
            _maxAnisotrophy = parameters.maxAnisotrophy;
            _comparisonFunction = parameters.comparisonFunction;
            _minLOD = parameters.minLOD;
            _maxLOD = parameters.maxLOD;
            _borderColor = parameters.borderColor;

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
            createInfo.magFilter = VulkanBackend::vulkanizedFilter(_min);
            createInfo.minFilter = VulkanBackend::vulkanizedFilter(_mag);
            createInfo.mipmapMode = VulkanBackend::vulkanizedMipMapMode(_mip);
            createInfo.addressModeU = VulkanBackend::vulkanizedAddressMode(_u);
            createInfo.addressModeV = VulkanBackend::vulkanizedAddressMode(_v);
            createInfo.addressModeW = VulkanBackend::vulkanizedAddressMode(_w);
            createInfo.mipLodBias = _mipLODBias;
            createInfo.anisotropyEnable = _anisotrophyEnabled;
            createInfo.maxAnisotropy = _maxAnisotrophy;
            createInfo.compareOp = VulkanBackend::vulkanizedCompareOp(_comparisonFunction);
            createInfo.minLod = _minLOD;
            createInfo.maxLod = _maxLOD;
            createInfo.pNext = &borderExt;

            auto result =vkCreateSampler(VulkanGraphicsCard::selected()->device(),&createInfo, nullptr,&_sampler);

            if(result!= VK_SUCCESS)
            {
                throw std::runtime_error("unable to create sampler");
            }
        }

        VulkanSampler::~VulkanSampler()
        {
             if(_sampler)
             {
                 vkDestroySampler(VulkanGraphicsCard::selected()->device(),_sampler, nullptr);
             }
        }

        void VulkanSampler::move(VulkanSampler& from)
        {
            std::swap(_sampler,from._sampler);
            std::swap(_min,from._min);
            std::swap(_mag,from._mag);
            std::swap(_u,from._u);
            std::swap(_v,from._v);
            std::swap(_w,from._w);
            std::swap(_mipLODBias,from._mipLODBias);
            std::swap(_anisotrophyEnabled,from._anisotrophyEnabled);
            std::swap(_maxAnisotrophy,from._maxAnisotrophy);
            std::swap(_comparisonFunction,from._comparisonFunction);
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

        VkSampler VulkanSampler::vulkanHandle()
        {
            return _sampler;
        }

        VulkanSampler::VulkanSampler(VulkanSampler&& from)
        {
            move(from);
        }

        VulkanSampler& VulkanSampler::operator=(VulkanSampler&& from)
        {
            move(from);
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
            return _maxAnisotrophy;
        }

        Operations::ComparisonFunction VulkanSampler::comparisonFunction()
        {
            return _comparisonFunction;
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
