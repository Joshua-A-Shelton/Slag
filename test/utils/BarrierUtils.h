#ifndef SLAG_BARRIERUTILS_H
#define SLAG_BARRIERUTILS_H
#include <slag/SlagLib.h>

namespace slag
{

    class BarrierUtils
    {
    public:
        static BarrierAccess compatibleAccess(Texture::Layout layout);
        static BarrierAccess compatibleAccess(Texture* texture);
        static BarrierAccess compatibleAccess(GpuQueue::QueueType queueType);
        static BarrierAccess compatibleAccess(Texture* texture, Texture::Layout toLayout, GpuQueue::QueueType queueType);
        static std::vector<Texture::Layout> compatibleLayouts(GpuQueue::QueueType queueType);
        static std::vector<Texture::Layout> compatibleLayouts(Texture* texture);
        static std::vector<Texture::Layout> compatibleLayouts(GpuQueue::QueueType queueType, Texture* texture);
    };

} // slag

#endif //SLAG_BARRIERUTILS_H
