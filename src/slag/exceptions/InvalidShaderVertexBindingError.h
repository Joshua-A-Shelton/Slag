#ifndef SLAG_INVALIDSHADERVERTEXBINDINGERROR_H
#define SLAG_INVALIDSHADERVERTEXBINDINGERROR_H
#include <stdexcept>


namespace slag
{
    class InvalidShaderVertexBindingError: public std::runtime_error
    {
    public:
        InvalidShaderVertexBindingError(const std::string& message):runtime_error(message){}
    };
}
#endif //SLAG_INVALIDSHADERVERTEXBINDINGERROR_H