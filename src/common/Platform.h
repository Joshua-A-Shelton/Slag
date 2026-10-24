#pragma once

// Small OS helpers used by common code.

namespace slag {

bool isDebuggerAttached();
void debugBreak();

// Writes a line to the debugger output (Windows) and stderr.
void writeDebugOutput(const char* text);

} // namespace slag
