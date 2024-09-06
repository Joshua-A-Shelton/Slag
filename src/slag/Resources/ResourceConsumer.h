#ifndef SLAG_RESOURCECONSUMER_H
#define SLAG_RESOURCECONSUMER_H
#include <mutex>
#include <unordered_set>

namespace slag
{
    namespace resources
    {

        class ResourceConsumer
        {
        public:
            virtual ~ResourceConsumer();
            friend class ResourceManager;
        protected:
            void freeResourceReferences();
            void move(ResourceConsumer& from);
        private:
            std::mutex _referencesMutex;
            std::unordered_set<void*> _resourceReferences;
            void addResourceReference(void* gpuID);
        };

    } // resources
} // slag

#endif //SLAG_RESOURCECONSUMER_H
