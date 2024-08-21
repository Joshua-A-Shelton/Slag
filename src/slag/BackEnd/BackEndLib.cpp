#include "BackEndLib.h"
namespace slag
{
    namespace lib
    {
        BackEndLib* currentBackEnd = nullptr;

        BackEndLib* BackEndLib::get()
        {
            return currentBackEnd;
        }

        void BackEndLib::set(BackEndLib* library)
        {
            currentBackEnd = library;
        }
    }
}