#pragma once
#include "runtime/function/render/ShaderCompiler/shader_compiler.h"
#include "runtime/function/render/ShaderCompiler/shader_compiler_cache.h"
#include <d3dcompiler.h>
#include "dxcapi.h"

namespace bocchi
{
    void throwIfFailed(HRESULT hr);

    bool initDirectXCompiler();
    bool dxComileToDxo(size_t            hash,
                       const char*       p_src_code,
                       const DefineList* p_defines,
                       const char*       p_entry_point,
                       const char*       p_params,
                       char**            out_spv_data,
                       size_t*           out_spv_size);
}

