#ifndef SLAG_DESCRIPTORMEMORY_H
#define SLAG_DESCRIPTORMEMORY_H
#include <cstdint>

namespace slag
{
    class DescriptorMemory
    {
    public:
        enum class Type
        {
            RESOURCE = 0,
            SAMPLER = 1,
        };
        virtual ~DescriptorMemory()=default;
        /**
         * What kind of descriptor this memory contains
         * @return
         */
        virtual Type type()=0;
        /**
         * Get the location for the next descriptor group closest to the given memory address. Some API's require descriptor sets to be aligned to a boundary, so this call is required
         * @param memoryOffset Location in memory to check for the next aligned descriptor group location for
         * @return
         */
        virtual uint64_t nextDescriptorGroupOffset(uint64_t memoryOffset)=0;
    };
}
#endif //SLAG_DESCRIPTORMEMORY_H