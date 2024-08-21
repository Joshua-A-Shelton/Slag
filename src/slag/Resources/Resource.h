#ifndef SLAG_RESOURCE_H
#define SLAG_RESOURCE_H
#include <functional>
namespace slag
{
    namespace resources
    {
        class Resource
        {
        public:
            Resource(bool destroyImmediately);
            virtual ~Resource()=default;
            virtual void* gpuID()=0;
            friend class ResourceManager;
        protected:
            void move(Resource& from);
            void smartDestroy();
            std::function<void()> _disposeFunction = nullptr;
            bool _destroyImmediately = false;
        };
    }// resources
} // slag

#endif //SLAG_RESOURCE_H
