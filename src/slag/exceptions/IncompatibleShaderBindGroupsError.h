#ifndef SLAG_INCOMPATIBLESHADERBINDGROUPSERROR_H
#define SLAG_INCOMPATIBLESHADERBINDGROUPSERROR_H
#include <stdexcept>


namespace slag
{
    class IncompatibleShaderBindGroupsError: public std::runtime_error
    {
    public:
        IncompatibleShaderBindGroupsError(const std::string& message):runtime_error(message){}
    };
}
#endif //SLAG_INCOMPATIBLESHADERBINDGROUPSERROR_H