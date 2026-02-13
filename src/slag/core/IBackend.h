#ifndef SLAG_IBACKEND_H
#define SLAG_IBACKEND_H
#include <cstdint>

namespace slag
{
    struct InitializationData;

    enum class BackendAPI
    {
        UNKNOWN,
        VULKAN,
        DX12,
        CUSTOM
    };
    enum class SlagInitializationResult
    {
        ///Backend was successfully found and initialized
        SUCCESS,
        ///Backend is not supported on the current platform with the current build of the library
        BACKEND_UNSUPPORTED,
        ///Backend is supported on the platform, but does not have the required capabilities required by SLAG. Users may be able to remedy by updating the backend
        INSUFFICIENT_CAPABILITIES,
        ///Backend is supported, but no graphics cards with required capabilities required by SLAG were found. Users may be able to remedy by updating graphics drivers
        NO_GRAPHICS_CARDS
    };
    class GraphicsCard;
    class IBackend
    {
    public:
        virtual ~IBackend() = default;
        ///Backend API type
        [[nodiscard]] virtual BackendAPI api()const;
        [[nodiscard]] virtual uint32_t graphicsCardCount()const;
        [[nodiscard]] virtual GraphicsCard* graphicsCard(uint32_t index);
    protected:
        friend class Slag;
        virtual SlagInitializationResult initializeBackend(const InitializationData& initializationData);
        IBackend() = default;
    };
} // slag

#endif //SLAG_IBACKEND_H
