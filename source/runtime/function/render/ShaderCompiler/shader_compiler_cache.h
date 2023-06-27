#pragma once
#include <string>

namespace bocchi
{
    bool        initShaderComplierCache(const std::string& shader_lib_dir, const std::string& shader_cache_dir);
    std::string getShaderCompilerLibDir(); 
    std::string getShaderCompilerCacheDir();
} // namespace Bocchi