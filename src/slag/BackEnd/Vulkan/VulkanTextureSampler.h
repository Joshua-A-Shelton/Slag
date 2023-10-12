#ifndef SLAG_VULKANTEXTURESAMPLER_H
#define SLAG_VULKANTEXTURESAMPLER_H
#include "../../TextureSampler.h"
#include "../Resource.h"
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {

        class VulkanTextureSampler: public TextureSampler, Resource
        {
        public:
            VulkanTextureSampler(TextureSampler::Filter minFilter, TextureSampler::Filter magFilter, TextureSampler::AddressMode u, TextureSampler::AddressMode v, TextureSampler::AddressMode w, bool destroyImmediate);
            ~VulkanTextureSampler()override;
            Filter magFilter()override;
            Filter minFilter()override;
            AddressMode addressU()override;
            AddressMode addressV()override;
            AddressMode addressW()override;
            void* GPUID()override;
            VkSampler vulkanSampler();
            static VkFilter filterFromCrossPlatform(TextureSampler::Filter filter);
            static VkSamplerAddressMode addressModeFromCrossPlatform(TextureSampler::AddressMode mode);
        private:
            VkSampler _sampler= nullptr;
            TextureSampler::Filter _min,_mag;
            TextureSampler::AddressMode _u,_v,_w;
        };

    } // slag
} // vulkan

#endif //SLAG_VULKANTEXTURESAMPLER_H
