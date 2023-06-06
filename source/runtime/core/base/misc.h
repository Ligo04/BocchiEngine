#pragma once
#include "runtime/core/base/macro.h"

namespace Bocchi
{
    std::string format(const char* format, ...);
    bool        lauchProcess(const char* command_line, const char* file_name_err);

    bool readFile(const char* name, char** data, size_t* size, bool isbinary);

    bool saveFile(const char* name, void const* data, size_t size, bool isbinary);
} // namespace Bocchi
