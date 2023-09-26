#include "VulkanVirtualUniformBuffer.h"

namespace slag
{
    namespace vulkan
    {
        VulkanVirtualUniformBuffer::VulkanVirtualUniformBuffer(VkDeviceSize defaultSize)
        {
            _backingBuffers.push_back(VulkanUniformBuffer(defaultSize));
            _virtualSize = _backingBuffers[0]._size;
        }

        void VulkanVirtualUniformBuffer::reset()
        {
            if(_backingBuffers.size()>1)
            {
                _backingBuffers.erase(_backingBuffers.begin()+1,_backingBuffers.end());
                _backingBuffers[0].reset(_virtualSize);
                _virtualSize = _backingBuffers[0]._size;
            }
            else
            {
                if(_backingBuffers[0]._offset < _backingBuffers[0].size()/2 && _backingBuffers[0]._offset!=0)
                {
                    VkDeviceSize newsize = _backingBuffers[0]._offset + (_backingBuffers[0]._offset/2);
                    _backingBuffers[0].reset(newsize);
                    _virtualSize = _backingBuffers[0]._size;
                }
                else
                {
                    _backingBuffers[0].reset();
                }
            }
            _currentBufferIndex = 0;
        }

        VulkanUniformBufferWriteData VulkanVirtualUniformBuffer::write(void *data, size_t size)
        {
            auto writeLocation = _backingBuffers[_currentBufferIndex].write(data,size);
            if(writeLocation)
            {
                return VulkanUniformBufferWriteData
                        {
                                _backingBuffers[_currentBufferIndex]._backingBuffer,
                                writeLocation.value(),
                                size
                        };
            }
            else
            {

                auto growth = std::max(_backingBuffers[_currentBufferIndex].size()/2,2*size);
                _currentBufferIndex++;
                _backingBuffers.push_back(VulkanUniformBuffer(growth));
                _virtualSize+= _backingBuffers.end()->size();
                return write(data,size);
            }
        }

        VkDeviceSize VulkanVirtualUniformBuffer::virtualSize()
        {
            return _virtualSize;
        }
    } // slag
} // vulkan