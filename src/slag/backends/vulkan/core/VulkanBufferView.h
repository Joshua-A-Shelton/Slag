#ifndef SLAG_VULKANBUFFERVIEW_H
#define SLAG_VULKANBUFFERVIEW_H
#include <slag/Slag.h>

#include "IVulkanCommandBuffer.h"
#include "slag/core/Pixels.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanBufferView: public BufferView
        {
        public:
            VulkanBufferView(Buffer* buffer, Pixels::Format format, uint64_t offset, uint64_t size);
            ~VulkanBufferView()override;
            VulkanBufferView(const VulkanBufferView&) = delete;
            VulkanBufferView& operator=(const VulkanBufferView&) = delete;
            VulkanBufferView(VulkanBufferView&& from);
            VulkanBufferView& operator=(VulkanBufferView&& from);

            virtual Buffer* buffer()override;
            virtual Pixels::Format format()override;
            virtual uint64_t offset()override;
            virtual uint64_t size()override;

            VkBufferView vulkanHandle() const;

        private:
            void move(VulkanBufferView& from);
            VkBufferView _bufferView = nullptr;
            Buffer* _buffer = nullptr;
            Pixels::Format _format = Pixels::Format::UNDEFINED;
            uint64_t _offset = 0;
            uint64_t _size = 0;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANBUFFERVIEW_H