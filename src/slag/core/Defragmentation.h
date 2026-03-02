#ifndef SLAG_DEFRAGMENTATION_H
#define SLAG_DEFRAGMENTATION_H
#include "Texture.h"

namespace slag
{
    enum class MemoryObjectType
    {
        BUFFER,
        TEXTURE
    };

    union MemoryObject
    {
        Buffer* buffer = nullptr;
        Texture* texture;
    };

    struct MemoryReference
    {
        MemoryObjectType type = MemoryObjectType::BUFFER;
        MemoryObject memory;
    };
}
#endif //SLAG_DEFRAGMENTATION_H