#include "Resource.h"
#include "ResourceManager.h"
#include <boost/uuid/uuid_generators.hpp>
namespace slag
{
    namespace resources
    {
        boost::uuids::random_generator RESOURCE_UUID_GENERATOR;
        Resource::Resource(bool destroyImmediately)
        {
            _destroyImmediately = destroyImmediately;
            _gpuID = RESOURCE_UUID_GENERATOR();
        }

        void Resource::smartDestroy()
        {
            if(_disposeFunction)
            {
                if(_destroyImmediately)
                {
                    _disposeFunction();
                }
                else
                {
                    resources::ResourceManager::queueResourceForDeletion((Resource*)this);
                }
            }
        }

        void Resource::smartMove()
        {
            if(_disposeFunction)
            {
                resources::ResourceManager::queueResourceForDeletion((Resource*)this);
                _gpuID = RESOURCE_UUID_GENERATOR();
            }
        }

        void Resource::move(Resource& from)
        {
            _destroyImmediately = from._destroyImmediately;
            from._destroyImmediately = true;
            _disposeFunction = from._disposeFunction;
            from._disposeFunction = nullptr;
            _gpuID = from._gpuID;
        }

        boost::uuids::uuid Resource::gpuID()
        {
            return _gpuID;
        }
    }//resources
} // slag