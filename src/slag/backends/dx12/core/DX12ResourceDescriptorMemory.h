#ifndef SLAG_DX12RESOURCEDESCRIPTORMEMORY_H
#define SLAG_DX12RESOURCEDESCRIPTORMEMORY_H
#include <slag/core/ResourceDescriptorMemory.h>

#include <d3d12.h>

namespace slag
{
    namespace dx12
    {
        class DX12ResourceDescriptorMemory: public ResourceDescriptorMemory
        {
        public:
            DX12ResourceDescriptorMemory(uint64_t descriptorCount);
            ~DX12ResourceDescriptorMemory()override;
            DX12ResourceDescriptorMemory(const DX12ResourceDescriptorMemory&) = delete;
            DX12ResourceDescriptorMemory& operator=(const DX12ResourceDescriptorMemory&) = delete;
            DX12ResourceDescriptorMemory(DX12ResourceDescriptorMemory&& from);
            DX12ResourceDescriptorMemory& operator=(DX12ResourceDescriptorMemory&& from);

            virtual uint64_t nextDescriptorGroupOffset(uint64_t memoryOffset)override;
            virtual uint64_t size()override;
            virtual uint64_t handle()override;

            virtual void setSampledTexture(uint64_t memoryLocation, Texture* texture)override;
            virtual void setStorageTexture(uint64_t memoryLocation, Texture* texture)override;
            virtual void setUniformTexelBuffer(uint64_t memoryLocation, Buffer* buffer, Pixels::Format format, uint64_t startIndex, uint64_t elements)override;
            virtual void setStorageTexelBuffer(uint64_t memoryLocation, Buffer* buffer, Pixels::Format format, uint64_t startIndex, uint64_t elementCount)override;
            virtual void setUniformBuffer(uint64_t memoryLocation, Buffer* buffer, uint64_t dataStride, uint64_t startIndex, uint64_t elementCount)override;
            virtual void setStorageBuffer(uint64_t memoryLocation, Buffer* buffer, uint64_t dataStride, uint64_t startIndex, uint64_t elementCount)override;
        private:
            void move(DX12ResourceDescriptorMemory& from);
            ID3D12DescriptorHeap* _heap = nullptr;
            uint64_t _descriptorSize = 0;
            uint64_t _size = 0;
        };
    } // dx12
} // slag

#endif //SLAG_DX12RESOURCEDESCRIPTORMEMORY_H
