#ifndef SLAG_SLAG_INVALIDSWAPCHAINOPERATION_H
#define SLAG_SLAG_INVALIDSWAPCHAINOPERATION_H
#include <stdexcept>

namespace slag
{
    class InvalidSwapChainOperation: public std::runtime_error
    {
    public:
        InvalidSwapChainOperation(const std::string& message):runtime_error(message){}
    };
}
#endif //SLAG_SLAG_INVALIDSWAPCHAINOPERATION_H