#include <cassert>
#include "VulkanVirtualUniformBuffer.h"
#include "VulkanUniformSet.h"
#include "VulkanLib.h"

namespace slag
{
    namespace vulkan
    {
        VulkanVirtualUniformBuffer::VulkanVirtualUniformBuffer(VkDeviceSize defaultSize, bool destroyImmediate)
        {
            if(defaultSize > 0)
            {
                _backingBuffers.push_back(VulkanUniformBuffer(defaultSize, destroyImmediate));
                _virtualSize = _backingBuffers[0]._size;
            }
            else
            {
                _virtualSize = 0;
            }
            _destroyImmediately = destroyImmediate;
        }

        VulkanVirtualUniformBuffer::VulkanVirtualUniformBuffer(VulkanVirtualUniformBuffer&& from)
        {
            move(std::move(from));
        }

        VulkanVirtualUniformBuffer& VulkanVirtualUniformBuffer::operator=(VulkanVirtualUniformBuffer&& from)
        {
            move(std::move(from));
            return *this;
        }

        void VulkanVirtualUniformBuffer::move(VulkanVirtualUniformBuffer&& from)
        {
            std::swap(_virtualSize,from._virtualSize);
            _backingBuffers.swap(from._backingBuffers);
            std::swap(_currentBufferIndex,from._currentBufferIndex);
            std::swap(_descriptorAllocator, from._descriptorAllocator);
            std::swap(_destroyImmediately,from._destroyImmediately);
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
            _descriptorAllocator.resetPools();
        }

        UniformData VulkanVirtualUniformBuffer::write(UniformSet* set, uint32_t uniformIndex, void* data, uint64_t size)
        {
            assert(_backingBuffers.size()>0 && "Use of invalid virtual buffer, no buffers have been assigned");

            VulkanUniformSet* uniformSet = static_cast<VulkanUniformSet*>(set);

            auto writeLocation = _backingBuffers[_currentBufferIndex].write(data,size);
            if(writeLocation)
            {
                VkDescriptorSet descriptorSet;
                auto success = _descriptorAllocator.allocate(&descriptorSet, uniformSet->descriptorSetLayout());
                assert(success == true && "unable to allocate descriptors");

                VkDescriptorBufferInfo buffer_info =
                        {
                                _backingBuffers[_currentBufferIndex]._backingBuffer,
                                writeLocation.value(),
                                size
                        };

                VkWriteDescriptorSet writeSet
                        {
                                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                nullptr,
                                descriptorSet,
                                uniformIndex,
                                0,
                                1,
                                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                                nullptr,
                                &buffer_info,
                                nullptr
                        };
                vkUpdateDescriptorSets(VulkanLib::graphicsCard()->device(),1,&writeSet,0, nullptr);
                return UniformData(_backingBuffers[_currentBufferIndex]._backingBuffer, descriptorSet, writeLocation.value(), size);
            }
            else
            {

                auto growth = std::max(_backingBuffers[_currentBufferIndex].size()/2,2*size);
                _currentBufferIndex++;
                _backingBuffers.push_back(VulkanUniformBuffer(growth,_destroyImmediately));
                _virtualSize+= _backingBuffers.end()->size();

                return write(set,uniformIndex,data,size);
            }
        }

        VkDeviceSize VulkanVirtualUniformBuffer::virtualSize()
        {
            return _virtualSize;
        }




    } // slag
} // vulkan