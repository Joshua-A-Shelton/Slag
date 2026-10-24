#pragma once

// Small OS helpers used by common code.

#include <cstdint>

namespace slag {

bool isDebuggerAttached();
void debugBreak();

// Host timestamps to nanoseconds: QueryPerformanceCounter ticks on Windows;
// CLOCK_MONOTONIC is already in nanoseconds elsewhere (ADR 0051).
uint64_t hostTicksToNanoseconds(uint64_t ticks);

// Writes a line to the debugger output (Windows) and stderr.
void writeDebugOutput(const char* text);

} // namespace slag
