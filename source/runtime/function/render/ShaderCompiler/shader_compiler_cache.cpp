#include "shader_compiler_cache.h"


namespace bocchi
{
    std::string s_shader_lib_dir;
    std::string s_shader_cache_dir;

    bool initShaderComplierCache(const std::string& shader_lib_dir,
                                 const std::string& shader_cache_dir)
    {
        s_shader_lib_dir   = shader_lib_dir;
        s_shader_cache_dir = shader_cache_dir;

        return true;
    }

    std::string getShaderCompilerLibDir() { return s_shader_lib_dir; }

    std::string getShaderCompilerCacheDir() { return s_shader_cache_dir; }

}
