#include "Resource.h"
#include "ResourceManager.h"
namespace slag
{
    namespace resources
    {
        Resource::Resource(bool destroyImmediately)
        {
            _destroyImmediately = destroyImmediately;
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

        void Resource::move(Resource& from)
        {
            _destroyImmediately = from._destroyImmediately;
            from._destroyImmediately = true;
            _disposeFunction = from._disposeFunction;
            from._disposeFunction = nullptr;
        }
    }//resources
} // slag