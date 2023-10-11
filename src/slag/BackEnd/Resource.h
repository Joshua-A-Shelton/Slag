#ifndef SLAG_RESOURCE_H
#define SLAG_RESOURCE_H

#include <functional>

namespace slag
{

    class Resource
    {
    public:
        virtual ~Resource() = default;
        virtual void* GPUID()=0;
    protected:
        void smartDestroy();
        bool destroyImmediately = false;
        std::function<void()> freeResources = nullptr;
    };
}
#endif //SLAG_RESOURCE_H
