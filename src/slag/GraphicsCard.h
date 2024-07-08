#ifndef SLAG_GRAPHICSCARD_H
#define SLAG_GRAPHICSCARD_H
#include <functional>
namespace slag
{
    struct CommandBuffer;
    class GraphicsCard
    {
    public:
        enum QueueType
        {
            GRAPHICS,
            TRANSFER,
            COMPUTE
        };
        virtual ~GraphicsCard()=default;
        //stay away from this unless you know what you're doing. It's used internally in some places, but it will stall GPU execution on the queue it's submitted to until it's finished
        virtual void executeArbitrary(std::function<void(CommandBuffer* commandBuffer)> execution,QueueType queue = GRAPHICS)=0;
    };
}
#endif //SLAG_GRAPHICSCARD_H