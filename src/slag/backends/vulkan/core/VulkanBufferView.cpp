#include "VulkanBufferView.h"

#include "VulkanBuffer.h"
#include "VulkanGraphicsCard.h"
#include "slag/backends/vulkan/VulkanBackend.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        VulkanBufferView::VulkanBufferView(Buffer* buffer, Pixels::Format format, uint64_t offset, uint64_t size)
        {
            SLAG_ASSERT(offset+size <= buffer->size() && "BufferView exceeds length of buffer");
            _buffer = buffer;
            _format = format;
            _offset = offset;
            _size = size;
            VkBufferViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            viewInfo.buffer = static_cast<VulkanBuffer*>(_buffer)->vulkanHandle();
            viewInfo.offset = _offset;
            viewInfo.range = _size;

            auto vulkanFormat = VulkanBackend::vulkanizedFormat(format);
            viewInfo.format = vulkanFormat.format;
            //TODO: I'm not sure what to do about the swizzling component mapping
            //vulkanFormat.mapping;
            vkCreateBufferView(VulkanGraphicsCard::selected()->device(),&viewInfo,nullptr,&_bufferView);
        }

        VulkanBufferView::~VulkanBufferView()
        {
            vkDestroyBufferView(VulkanGraphicsCard::selected()->device(),_bufferView,nullptr);
        }

        Buffer* VulkanBufferView::buffer()
        {
            return _buffer;
        }

        Pixels::Format VulkanBufferView::format()
        {
            return _format;
        }

        uint64_t VulkanBufferView::offset()
        {
            return _offset;
        }

        uint64_t VulkanBufferView::size()
        {
            return _size;
        }

        VkBufferView VulkanBufferView::vulkanHandle() const
        {
            return _bufferView;
        }
    } // vulkan
} // slag