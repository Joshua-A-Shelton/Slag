#ifndef SLAG_RESOURCECREATIONERROR_H
#define SLAG_RESOURCECREATIONERROR_H
#include <stdexcept>

namespace slag
{
    class ResourceCreationError: public std::runtime_error
    {
    public:
        ResourceCreationError(const std::string& message):runtime_error(message){}
    };
} // slag

#endif //SLAG_RESOURCECREATIONERROR_H
