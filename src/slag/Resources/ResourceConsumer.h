#ifndef SLAG_RESOURCECONSUMER_H
#define SLAG_RESOURCECONSUMER_H
#include <mutex>
#include <unordered_set>
#include <boost/uuid/uuid.hpp>
#include <boost/container_hash/hash.hpp>

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
            std::unordered_set<boost::uuids::uuid,boost::hash<boost::uuids::uuid>> _resourceReferences;
            void addResourceReference(boost::uuids::uuid gpuID);
        };

    } // resources
} // slag

#endif //SLAG_RESOURCECONSUMER_H
