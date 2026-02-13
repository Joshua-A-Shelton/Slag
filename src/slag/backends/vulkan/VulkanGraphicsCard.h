#ifndef SLAG_VULKANGRAPHICSCARD_H
#define SLAG_VULKANGRAPHICSCARD_H
#include <slag/Slag.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>

namespace slag
{
    namespace vulkan
    {
        class VulkanGraphicsCard: public GraphicsCard
        {
        public:
            VulkanGraphicsCard(VkInstance instance, const vkb::Device& device);
            ~VulkanGraphicsCard()override;
            VulkanGraphicsCard(const VulkanGraphicsCard&) = delete;
            VulkanGraphicsCard& operator=(const VulkanGraphicsCard&) = delete;
            VulkanGraphicsCard(VulkanGraphicsCard&& from);
            VulkanGraphicsCard& operator=(VulkanGraphicsCard&& from);
            [[nodiscard]] std::string name()const override;
            [[nodiscard]] uint64_t videoMemory()const override;
            [[nodiscard]] bool cacheCoherentSharedMemory()const override;
            //Buffers
            /**
             * Allocate a new buffer
             * @param size Size in bytes of the buffer
             * @param usage Kind of data will the buffer store
             * @param shaderAccess Data access permissions for shaders
             * @param cpuAccess Data access permissions for the cpu
             * @return
             */
            [[nodiscard]] Buffer* newBuffer(
                uint64_t size,
                BufferUsage usage = BufferUsage::ARBITRARY,
                BufferShaderAccess shaderAccess = BufferShaderAccess::READ_WRITE,
                BufferCPUAccess cpuAccess = BufferCPUAccess::WRITE_ONLY)override;
        private:
            void move(VulkanGraphicsCard& from);
            std::string _name;
            VkPhysicalDevice _physicalDevice=nullptr;
            VkDevice _device=nullptr;
            VkPhysicalDeviceProperties _physicalDeviceProperties={};
            VkPhysicalDeviceMemoryProperties _physicalDeviceMemoryProperties={};
            VmaAllocator _allocator = nullptr;
            uint64_t _videoMemory = 0;
            bool _cacheCoherentSharedMemory = false;

        };
    } // vulkan
} // slag

#endif //SLAG_VULKANGRAPHICSCARD_H
