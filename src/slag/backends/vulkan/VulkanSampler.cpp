#include "VulkanSampler.h"

#include "VulkanBackend.h"
#include "VulkanGraphicsCard.h"
#include "slag/exceptions/ResourceCreationError.h"

namespace slag
{
    namespace vulkan
    {
        VulkanSampler::VulkanSampler(VulkanGraphicsCard* graphicsCard, SamplerFilter min, SamplerFilter mag, SamplerFilter mip, SamplerAddressMode u,
            SamplerAddressMode v, SamplerAddressMode w, float mipLODBias, bool anisotrophyEnabled,
            uint8_t maxAnisotrophy, ComparisonFunction comparisonFunction, Color borderColor, float minLOD,
            float maxLOD)
        {
            _graphicsCard = graphicsCard;
            _min = min;
            _mag = mag;
            _mip = mip;
            _u = u;
            _v = v;
            _w = w;
            _mipLODBias = mipLODBias;
            _anisotrophyEnabled = anisotrophyEnabled;
            _maxAnisotrophy = maxAnisotrophy;
            _comparisonFunction = comparisonFunction;
            _borderColor = borderColor;
            _minLOD = minLOD;
            _maxLOD = maxLOD;


            _createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            _createInfo.magFilter = VulkanBackend::nativeFilter(_min);
            _createInfo.minFilter = VulkanBackend::nativeFilter(_mag);
            _createInfo.mipmapMode = (VkSamplerMipmapMode)VulkanBackend::nativeFilter(_mip);
            _createInfo.addressModeU = VulkanBackend::nativeSamplerAddressMode(_u);
            _createInfo.addressModeV = VulkanBackend::nativeSamplerAddressMode(_v);
            _createInfo.addressModeW = VulkanBackend::nativeSamplerAddressMode(_w);
            _createInfo.mipLodBias = _mipLODBias;
            _createInfo.anisotropyEnable = _anisotrophyEnabled;
            _createInfo.maxAnisotropy = _maxAnisotrophy;
            _createInfo.compareOp = VulkanBackend::nativeCompareOp(_comparisonFunction);
            _createInfo.minLod = _minLOD;
            _createInfo.maxLod = _maxLOD;
            //TODO: figure out what I have to do to have border color
            _createInfo.pNext = &_borderExt;

            VkSamplerCustomBorderColorCreateInfoEXT borderExt
            {
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT,
                .customBorderColor = {.float32 = {borderColor.red,borderColor.green,borderColor.blue,borderColor.alpha}},
            };
            _graphicsCard->vkRegisterCustomBorderColor(_graphicsCard->device(),&borderExt,true,&_borderExt.index);

        }

        VulkanSampler::~VulkanSampler()
        {
            if (_graphicsCard)
            {
                _graphicsCard->vkUnregisterCustomBorderColor(_graphicsCard->device(),_borderExt.index);
            }
        }

        VulkanSampler::VulkanSampler(VulkanSampler&& from) noexcept
        {
            move(from);
        }

        VulkanSampler& VulkanSampler::operator=(VulkanSampler&& from) noexcept
        {
            move(from);
            return *this;
        }

        SamplerFilter VulkanSampler::magFilter()
        {
            return _mag;
        }

        SamplerFilter VulkanSampler::minFilter()
        {
            return _min;
        }

        SamplerFilter VulkanSampler::mipMapFilter()
        {
            return _mip;
        }

        SamplerAddressMode VulkanSampler::addressU()
        {
            return _u;
        }

        SamplerAddressMode VulkanSampler::addressV()
        {
            return _v;
        }

        SamplerAddressMode VulkanSampler::addressW()
        {
            return _w;
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

        ComparisonFunction VulkanSampler::comparisonFunction()
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

        GraphicsCard* VulkanSampler::graphicsCard()
        {
            return _graphicsCard;
        }

        VkSamplerCreateInfo VulkanSampler::vulkanHandle() const
        {
            return _createInfo;
        }

        void VulkanSampler::move(VulkanSampler& from)
        {
            std::swap(_createInfo,from._createInfo);
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
            std::swap(_maxLOD,from._maxLOD);
            std::swap(_borderColor,from._borderColor);
            std::swap(_graphicsCard,from._graphicsCard);

            _createInfo.pNext = &_borderExt;
        }
    } // vulkan
} // slag