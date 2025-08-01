#include "VulkanDescriptorPool.h"

#include "VulkanDescriptorGroup.h"
#include "VulkanGraphicsCard.h"
namespace slag
{
    namespace vulkan
    {
        VulkanDescriptorPool::VulkanDescriptorPool(const DescriptorPoolPageInfo& pageInfo)
        {
            _pageInfo = pageInfo;
            _pages.push_back(allocatePage());
        }

        VulkanDescriptorPool::~VulkanDescriptorPool()
        {
            auto device = VulkanGraphicsCard::selected()->device();
            for(size_t i=0; i< _pages.size(); i++)
            {
                vkDestroyDescriptorPool(device,_pages[i], nullptr);
            }
        }

        VulkanDescriptorPool::VulkanDescriptorPool(VulkanDescriptorPool&& from)
        {
            move(std::move(from));
        }

        VulkanDescriptorPool& VulkanDescriptorPool::operator=(VulkanDescriptorPool&& from)
        {
            move(std::move(from));
            return *this;
        }

        void VulkanDescriptorPool::move(VulkanDescriptorPool&& from)
        {
            _pages.swap(from._pages);
            _currentPage = from._currentPage;
            _pageInfo = from._pageInfo;
        }

        void VulkanDescriptorPool::reset()
        {
            auto device = VulkanGraphicsCard::selected()->device();
            for(auto i=0; i< _pages.size(); i++)
            {
                vkResetDescriptorPool(device,_pages[i],0);
            }
            _currentPage=0;
        }

        void VulkanDescriptorPool::setBundleLowLevelHandles(void** gpuHandle, void** cpuHandle, DescriptorGroup* forGroup)
        {
            VkDescriptorPool page = _pages[_currentPage];
            auto group = static_cast<VulkanDescriptorGroup*>(forGroup);
            auto layout = group->layout();
            VkDescriptorSet handle;
            VkDescriptorSetAllocateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            info.descriptorPool = page;
            info.descriptorSetCount = 1;
            info.pSetLayouts = &layout;
            auto result = vkAllocateDescriptorSets(VulkanGraphicsCard::selected()->device(),&info,&handle);

            bool needReallocate = false;

            switch (result) {
                case VK_SUCCESS:
                    //all good, return
                    *gpuHandle = handle;
                    return;
                case VK_ERROR_FRAGMENTED_POOL:
                case VK_ERROR_OUT_OF_POOL_MEMORY:
                    //reallocate pool
                    needReallocate = true;
                    break;
                default:
                    //unrecoverable error
                    throw std::runtime_error("Unable to create bundle in pool");
            }

            if (needReallocate)
            {
                if(_currentPage == _pages.size()-1)
                {
                    page = allocatePage();
                    _pages.push_back(page);
                    info.descriptorPool = page;
                    _currentPage++;
                }
                else
                {
                    _currentPage++;
                    info.descriptorPool = _pages[_currentPage];
                }
                result = vkAllocateDescriptorSets(VulkanGraphicsCard::selected()->device(),&info,&handle);
                //if it still fails then we have big issues
                if (result != VK_SUCCESS)
                {
                    throw std::runtime_error("Unable to create bundle in newly allocated page. Out of memory or page insufficient for bundle size");
                }
            }
            *gpuHandle = handle;
        }

        VkDescriptorPool VulkanDescriptorPool::allocatePage()
        {
            std::vector<VkDescriptorPoolSize> sizes;
            if(_pageInfo.samplers)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_SAMPLER,_pageInfo.samplers});
            }
            if(_pageInfo.sampledTextures)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,_pageInfo.sampledTextures});
            }
            if(_pageInfo.combinedSamplerTextures)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,_pageInfo.combinedSamplerTextures});
            }
            if(_pageInfo.storageTextures)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,_pageInfo.storageTextures});
            }
            if(_pageInfo.uniformTexelBuffers)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,_pageInfo.uniformTexelBuffers});
            }
            if(_pageInfo.storageTexelBuffers)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,_pageInfo.storageTexelBuffers});
            }
            if(_pageInfo.uniformBuffers)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,_pageInfo.uniformBuffers});
            }
            if(_pageInfo.storageBuffers)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,_pageInfo.storageBuffers});
            }
            if(_pageInfo.inputAttachments)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,_pageInfo.inputAttachments});
            }
            if(_pageInfo.accelerationStructures)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,_pageInfo.accelerationStructures});
            }

            VkDescriptorPoolCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            //TODO: make this number.... correct
            info.maxSets = _pageInfo.descriptorBundles;//arbitrary
            info.pPoolSizes = sizes.data();
            info.poolSizeCount = sizes.size();

            VkDescriptorPool _page{};
            auto success = vkCreateDescriptorPool(VulkanGraphicsCard::selected()->device(),&info, nullptr,&_page);
            if(success == VK_SUCCESS)
            {
                return _page;
            }
            else
            {
                throw std::runtime_error("Unable to allocate new descriptor pool page");
            }
        }
    } // vulkan
} // slag
