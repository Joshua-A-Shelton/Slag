#ifndef SLAG_RESOURCE_H
#define SLAG_RESOURCE_H

#include <functional>

namespace slag
{

    class Resource
    {
    public:
        virtual ~Resource()=default;
        virtual void* GPUID()=0;
        Resource()=default;
    protected:
        bool destroyImmediately = false;
        std::function<void()> freeResources = nullptr;
        Resource(Resource&& from);
        Resource& operator=(Resource&& from);
        Resource(const Resource&)=delete;
        Resource& operator=(const Resource&)=delete;
        void smartDestroy();
    };
}
#endif //SLAG_RESOURCE_H
