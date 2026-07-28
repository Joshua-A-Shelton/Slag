#ifndef SLAG_INVALIDSHADERCODEERROR_H
#define SLAG_INVALIDSHADERCODEERROR_H
#include <stdexcept>


namespace slag
{
    class InvalidShaderCodeError: public std::runtime_error
    {
    public:
        InvalidShaderCodeError(const std::string& message):runtime_error(message){}
    };
}
#endif //SLAG_INVALIDSHADERCODEERROR_H