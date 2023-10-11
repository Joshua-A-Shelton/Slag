#include "VulkanDescriptorAllocator.h"
#include "VulkanLib.h"
namespace slag
{
    namespace vulkan
    {
        VulkanDescriptorAllocator::VulkanDescriptorAllocator()
        {

        }

        VulkanDescriptorAllocator::~VulkanDescriptorAllocator()
        {
            //delete every pool held
            for (auto p : freePools)
            {
                vkDestroyDescriptorPool(VulkanLib::graphicsCard()->device(), p, nullptr);
            }
            for (auto p : usedPools)
            {
                vkDestroyDescriptorPool(VulkanLib::graphicsCard()->device(), p, nullptr);
            }
        }

        VkDescriptorPool VulkanDescriptorAllocator::createPool(VkDevice device, const VulkanDescriptorAllocator::PoolSizes &poolSizes, int count, VkDescriptorPoolCreateFlags flags)
        {
            std::vector<VkDescriptorPoolSize> sizes;
            sizes.reserve(poolSizes.sizes.size());
            for (auto sz : poolSizes.sizes) {
                sizes.push_back({ sz.first, uint32_t(sz.second * count) });
            }
            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = flags;
            pool_info.maxSets = count;
            pool_info.poolSizeCount = (uint32_t)sizes.size();
            pool_info.pPoolSizes = sizes.data();

            VkDescriptorPool descriptorPool;
            vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool);

            return descriptorPool;
        }

        VkDescriptorPool VulkanDescriptorAllocator::grab_pool()
        {
            //there are reusable pools availible
            if (freePools.size() > 0)
            {
                //grab pool from the back of the vector and remove it from there.
                VkDescriptorPool pool = freePools.back();
                freePools.pop_back();
                return pool;
            }
            else
            {
                //no pools availible, so create a new one
                return createPool(VulkanLib::graphicsCard()->device(), descriptorSizes, 1000, 0);
            }
        }

        bool VulkanDescriptorAllocator::allocate(VkDescriptorSet *set, VkDescriptorSetLayout layout)
        {
            //initialize the currentPool handle if it's null
            if (currentPool == VK_NULL_HANDLE){

                currentPool = grab_pool();
                usedPools.push_back(currentPool);
            }

            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.pNext = nullptr;

            allocInfo.pSetLayouts = &layout;
            allocInfo.descriptorPool = currentPool;
            allocInfo.descriptorSetCount = 1;

            //try to allocate the descriptor set
            VkResult allocResult = vkAllocateDescriptorSets(VulkanLib::graphicsCard()->device(), &allocInfo, set);
            bool needReallocate = false;

            switch (allocResult) {
                case VK_SUCCESS:
                    //all good, return
                    return true;
                case VK_ERROR_FRAGMENTED_POOL:
                case VK_ERROR_OUT_OF_POOL_MEMORY:
                    //reallocate pool
                    needReallocate = true;
                    break;
                default:
                    //unrecoverable error
                    return false;
            }

            if (needReallocate){
                //allocate a new pool and retry
                currentPool = grab_pool();
                usedPools.push_back(currentPool);

                allocResult = vkAllocateDescriptorSets(VulkanLib::graphicsCard()->device(), &allocInfo, set);

                //if it still fails then we have big issues
                if (allocResult == VK_SUCCESS){
                    return true;
                }
            }

            return false;
        }

        void VulkanDescriptorAllocator::resetPools()
        {
            //reset all used pools and add them to the free pools
            for (auto p : usedPools){
                vkResetDescriptorPool(VulkanLib::graphicsCard()->device(), p, 0);
                freePools.push_back(p);
            }

            //clear the used pools, since we've put them all in the free pools
            usedPools.clear();

            //reset the current pool handle back to null
            currentPool = VK_NULL_HANDLE;
        }

        VulkanDescriptorAllocator::VulkanDescriptorAllocator(VulkanDescriptorAllocator &&from)
        {
            move(std::move(from));
        }

        VulkanDescriptorAllocator &VulkanDescriptorAllocator::operator=(VulkanDescriptorAllocator &&from)
        {
            move(std::move(from));
            return *this;
        }

        void VulkanDescriptorAllocator::move(VulkanDescriptorAllocator &&from)
        {
            std::swap(currentPool,from.currentPool);
            std::swap(descriptorSizes,from.descriptorSizes);
            usedPools.swap(from.usedPools);
            freePools.swap(from.freePools);

        }


    } // slag
} // vulkan