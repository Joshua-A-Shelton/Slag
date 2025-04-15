#ifndef SLAG_RESOURCE_H
#define SLAG_RESOURCE_H
#include <functional>
#include <boost/uuid/uuid.hpp>
namespace slag
{
    namespace resources
    {
        class Resource
        {
        public:
            Resource(bool destroyImmediately);
            virtual ~Resource()=default;
            boost::uuids::uuid gpuID();
            friend class ResourceManager;
        private:
            boost::uuids::uuid _gpuID;
        protected:
            void move(Resource& from);
            void smartDestroy();
            void smartMove();

            std::function<void()> _disposeFunction = nullptr;
            bool _destroyImmediately = false;
        };
    }// resources
} // slag

#endif //SLAG_RESOURCE_H
