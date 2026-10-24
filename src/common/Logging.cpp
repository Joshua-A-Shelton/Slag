#include "common/Logging.h"

#include "common/Objects.h"
#include "common/Platform.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

namespace slag {

namespace {

constexpr size_t maxMessageLength = 1024;

void emit(const MessageSink& sink, MessageSeverity severity, MessageSource source, const char* text)
{
    if (sink.callback)
    {
        sink.callback(severity, source, text, sink.userData);
        return;
    }
    // Without a callback, only errors and warnings are printed (ADR 0011).
    if (sink.printWithoutCallback && severity != MessageSeverity::Info)
    {
        writeDebugOutput(text);
    }
}

bool printByDefault(ValidationFlags validation)
{
#ifdef SLAG_VALIDATION
    (void)validation;
    return true;
#else
    return validation != ValidationFlags::None;
#endif
}

} // namespace

MessageSink messageSink(const InstanceDesc& desc)
{
    return {desc.messageCallback, desc.messageUserData, printByDefault(desc.validationFlags)};
}

MessageSink messageSink(const InstanceCommon* instance)
{
    if (!instance)
    {
        return {};
    }
    return {instance->messageCallback, instance->messageUserData, printByDefault(instance->validationFlags)};
}

void logMessage(const MessageSink& sink, MessageSeverity severity, MessageSource source, const char* format, ...)
{
    if (!sink.callback && !sink.printWithoutCallback)
    {
        return;
    }
    char text[maxMessageLength];
    va_list args;
    va_start(args, format);
    std::vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    emit(sink, severity, source, text);
}

void logText(const MessageSink& sink, MessageSeverity severity, MessageSource source, const char* text)
{
    if (sink.callback || sink.printWithoutCallback)
    {
        emit(sink, severity, source, text);
    }
}

void validationFailure(const InstanceCommon* instance, const char* function, const char* format, ...)
{
    char detail[maxMessageLength];
    va_list args;
    va_start(args, format);
    std::vsnprintf(detail, sizeof(detail), format, args);
    va_end(args);

    // Room for the prefix and function name on top of the detail.
    char text[maxMessageLength + 256];
    std::snprintf(text, sizeof(text), "slag: %s: %s", function, detail);

    MessageSink sink = messageSink(instance);
    sink.printWithoutCallback = true;
    emit(sink, MessageSeverity::Error, MessageSource::Slag, text);

    ValidationFailureAction action = instance ? instance->failureAction : ValidationFailureAction::Break;
    switch (action)
    {
    case ValidationFailureAction::Break:
        if (isDebuggerAttached())
        {
            debugBreak();
        }
        break;
    case ValidationFailureAction::Continue:
        break;
    case ValidationFailureAction::Abort:
        std::abort();
    }
}

} // namespace slag
