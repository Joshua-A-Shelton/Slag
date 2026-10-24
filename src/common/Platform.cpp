#include "common/Platform.h"

#include <cstdio>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <csignal>
#endif

namespace slag {

bool isDebuggerAttached()
{
#ifdef _WIN32
    return IsDebuggerPresent() != 0;
#else
    // Linux: a non-zero TracerPid means a debugger is attached.
    std::FILE* status = std::fopen("/proc/self/status", "r");
    if (!status)
    {
        return false;
    }
    char line[256];
    bool attached = false;
    while (std::fgets(line, sizeof(line), status))
    {
        if (std::strncmp(line, "TracerPid:", 10) == 0)
        {
            int pid = 0;
            attached = std::sscanf(line + 10, "%d", &pid) == 1 && pid != 0;
            break;
        }
    }
    std::fclose(status);
    return attached;
#endif
}

void debugBreak()
{
#ifdef _WIN32
    __debugbreak();
#else
    std::raise(SIGTRAP);
#endif
}

void writeDebugOutput(const char* text)
{
#ifdef _WIN32
    OutputDebugStringA(text);
    OutputDebugStringA("\n");
#endif
    std::fprintf(stderr, "%s\n", text);
}

} // namespace slag
