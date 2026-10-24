#pragma once

// Debug messages (ADR 0011) and validation failures (ADR 0067).

#include <slag/Instance.h>

#if defined(__GNUC__) || defined(__clang__)
#define SLAG_PRINTF_FORMAT(formatIndex, firstArgIndex) __attribute__((format(printf, formatIndex, firstArgIndex)))
#else
#define SLAG_PRINTF_FORMAT(formatIndex, firstArgIndex)
#endif

namespace slag {

struct InstanceCommon;

// Where messages go: the user's callback, or stderr / the debugger output when
// there is no callback and printing is enabled.
struct MessageSink
{
    DebugMessageFn callback = nullptr;
    void* userData = nullptr;
    bool printWithoutCallback = false;
};

MessageSink messageSink(const InstanceDesc& desc);
MessageSink messageSink(const InstanceCommon* instance);

void logMessage(const MessageSink& sink, MessageSeverity severity, MessageSource source, const char* format, ...)
    SLAG_PRINTF_FORMAT(4, 5);

// Reports a failed Slag validation check, then applies the instance's
// ValidationFailureAction. The failing call still proceeds.
void validationFailure(const InstanceCommon* instance, const char* function, const char* format, ...)
    SLAG_PRINTF_FORMAT(3, 4);

} // namespace slag

#ifdef SLAG_VALIDATION
#define SLAG_VALIDATE(instance, condition, function, ...)                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            ::slag::validationFailure((instance), (function), __VA_ARGS__);                                            \
        }                                                                                                              \
    } while (false)
#else
#define SLAG_VALIDATE(instance, condition, function, ...)                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (false)
#endif
