#include "runtime/core/base/asyncCache.h"
#include "runtime/function/render/ShaderCompiler/dxc_helper.h"
#include "runtime/core/base/misc.h"
#include "shader_compiler_helper.h"
#include "shader_compiler_cache.h"
#include <codecvt>
#include <fstream>


namespace bocchi
{
    std::wstring shader_cache_path = L"";

    Cache<VkShaderModule> s_shader_cache;

    // generate sources form the input data
    std::string generateSource(ShaderSourceType            source_type,
                               const VkShaderStageFlagBits shader_type,
                               const char*                 p_shader,
                               const char*                 shader_compile_params,
                               const DefineList*           p_defines)
    {
        std::string shader_code(p_shader);
        std::string code;

        if (source_type == SST_GLSL)
        {
            // the first line in a GLSL shader must be the #version, insert the #defines right after this line
            size_t index = shader_code.find_first_of('\n');
            code         = shader_code.substr(index, shader_code.size() - index);

            shader_code = shader_code.substr(0, index) + "\n";
        }
        else if (source_type == SST_HLSL)
        {
            code        = shader_code;
            shader_code = "";
        }

        // add the #defines to the code to help debugging
        if (p_defines)
        {
            for (auto it = p_defines->begin(); it != p_defines->end(); ++it)
            {
                shader_code += "#define " + it->first + " " + it->second + "\n";
            }
        }

        // concar the actual shader code
        shader_code += code;

        return code;
    }

        //
    // Compiles a shader into SpirV
    //
    bool vkCompileToSpirv(size_t                      hash,
                          ShaderSourceType            source_type,
                          const VkShaderStageFlagBits shader_type,
                          const std::string&          shader_code,
                          const char*                 p_shader_entry_point,
                          const char*                 shader_compiler_params,
                          const DefineList*           p_defines,
                          char**                      out_spv_data,
                          size_t*                     out_spv_size
    ) 
    {
        // create glsl file for shader compiler to compile
        //
        std::string filename_spv;
        std::string filename_glsl;
        if (source_type == SST_GLSL)
        {
            filename_spv  = format("%s\\%p.spv", getShaderCompilerCacheDir().c_str(), hash);
            filename_glsl = format("%s\\%p.glsl", getShaderCompilerCacheDir().c_str(), hash);
        }
        else if (source_type == SST_HLSL)
        {
            filename_spv  = format("%s\\%p.dxo", getShaderCompilerCacheDir().c_str(), hash);
            filename_glsl = format("%s\\%p.hlsl", getShaderCompilerCacheDir().c_str(), hash);
        }
        else
            assert(!"unknown shader extension");

        std::ofstream ofs(filename_glsl, std::ofstream::out);
        ofs << shader_code;
        ofs.close();

        // compute command line to invoke the shader compiler
        //
        char* stage = NULL;
        switch (shader_type)
        {
            case VK_SHADER_STAGE_VERTEX_BIT:
                stage = "vertex";
                break;
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                stage = "fragment";
                break;
            case VK_SHADER_STAGE_COMPUTE_BIT:
                stage = "compute";
                break;
			default: ;
        }

        // add the #defines
        //
        std::string defines;
        if (p_defines)
        {
            for (auto it = p_defines->begin(); it != p_defines->end(); ++it)
                defines += "-D" + it->first + "=" + it->second + " ";
        }
        std::string command_line;
        if (source_type == SST_GLSL)
        {
            command_line =
                format("glslc --target-env=vulkan1.1 -fshader-stage=%s -fentry-point=%s %s \"%s\" -o \"%s\" -I %s %s",
                       stage,
                       p_shader_entry_point,
                       shader_compiler_params,
                       filename_glsl.c_str(),
                       filename_spv.c_str(),
                       getShaderCompilerLibDir().c_str(),
                       defines.c_str());

            std::string filename_err = format("%s\\%p.err", getShaderCompilerCacheDir().c_str(), hash);

            if (lauchProcess(command_line.c_str(), filename_err.c_str()) == true)
            {
                readFile(filename_spv.c_str(), out_spv_data, out_spv_size, true);
                assert(*out_spv_size != 0);
                return true;
            }
        }
        else
        {
            std::string scp = format("-spirv -fspv-target-env=vulkan1.1 -I %s %s %s",
                                     getShaderCompilerLibDir().c_str(),
                                     defines.c_str(),
                                     shader_compiler_params);
            dxComileToDxo(hash, shader_code.c_str(), p_defines, p_shader_entry_point, scp.c_str(), out_spv_data, out_spv_size);
            assert(*out_spv_size != 0);

            return true;
        }

        return false;
    }

    /**
     * \brief create the shader cache
     */
    void createShaderCache()
    {
        std::wstring s_shader_cache_path_w = shader_cache_path + L"\\ShaderCacheVK";



        initShaderComplierCache("ShaderLibVK",
                                std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(s_shader_cache_path_w));
    }

    void destoryShaderCache(VkDevice device)
    {
        s_shader_cache.ForEach([device](const Cache<VkShaderModule>::DatabaseType::iterator& it) {
            vkDestroyShaderModule(device, it->second.m_data, nullptr);
        });
    }

    VkResult createModule(VkDevice device, char* spv_data, size_t spv_size, VkShaderModule* p_shader_module)
    {
        VkShaderModuleCreateInfo module_create_info {};
        module_create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        module_create_info.pCode    = (uint32_t*)spv_data;
        module_create_info.codeSize = spv_size;

        return vkCreateShaderModule(device, &module_create_info, nullptr, p_shader_module);
    }

    VkResult vkCompile(VkDevice                         device,
                       ShaderSourceType                 source_type,
                       const VkShaderStageFlagBits      shader_type,
                       const char*                      p_shader,
                       const char*                      p_shader_entry_point,
                       const char*                      shader_compiler_params,
                       const DefineList*                p_defines,
                       VkPipelineShaderStageCreateInfo* p_shader_create_info)
    {
        VkResult res = VK_SUCCESS;

        // compute hash

        size_t hash_res;
        hash_res = hashShaderString((getShaderCompilerLibDir() + "\\").c_str(), p_shader);
        hash_res = hash(p_shader_entry_point, strlen(p_shader_entry_point), hash_res);
        hash_res = hash(shader_compiler_params, strlen(shader_compiler_params), hash_res);
        hash_res = hash((char*)&shader_type, sizeof(shader_type), hash_res);

        if (p_defines != NULL)
        {
            hash_res = p_defines->hash(hash_res);
        }

#define USE_MULTIHREADED_CACHE
#ifdef USE_MULTIHREADED_CACHE
        // compile if not in cache
        if (s_shader_cache.CacheMiss(hash_res, &p_shader_create_info->module))
#endif
        {
            char*  spv_data = NULL;
            size_t spv_size = 0;

#ifdef USE_SPIRV_FROM_DISK
            std::string filenameSpv = format("%s\\%p.spv", GetShaderCompilerCacheDir().c_str(), hash);
            if (ReadFile(filenameSpv.c_str(), &SpvData, &SpvSize, true) == false)
#endif
            {
                std::string shader =
                    generateSource(source_type, shader_type, p_shader, shader_compiler_params, p_defines);
                
            }

            assert(spv_size != 0);

            createModule(device, spv_data, spv_size, &p_shader_create_info->module);
            free(spv_data);

#ifdef USE_MULTIHREADED_CACHE
            s_shader_cache.UpdateCache(hash_res, &p_shader_create_info->module);
#endif
        }

        p_shader_create_info->sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        p_shader_create_info->pNext               = NULL;
        p_shader_create_info->pSpecializationInfo = NULL;
        p_shader_create_info->flags               = 0;
        p_shader_create_info->stage               = shader_type;
        p_shader_create_info->pName               = p_shader_entry_point;

        return res;
    }
    

    VkResult vkCompileFromString(VkDevice                         device,
                                 ShaderSourceType                 source_type,
                                 const VkShaderStageFlagBits      shader_type,
                                 const char*                      p_shader_code,
                                 const char*                      p_shader_entry_point,
                                 const char*                      shader_compiler_params,
                                 const DefineList*                p_defines,
                                 VkPipelineShaderStageCreateInfo* p_shader_create_info)
    {
        assert(strlen(p_shader_code) > 0);

        VkResult res = vkCompile(device,
                                 source_type,
                                 shader_type,
                                 p_shader_code,
                                 p_shader_entry_point,
                                 shader_compiler_params,
                                 p_defines,
                                 p_shader_create_info);
        assert(res == VK_SUCCESS);

        return res;
    }

    VkResult vkCompileFromFile(VkDevice                         device,
                               const VkShaderStageFlagBits      shader_type,
                               const char*                      p_shder_file_name,
                               const char*                      p_shader_entry_point,
                               const char*                      p_shader_compiler_params,
                               const DefineList*                p_defines,
                               VkPipelineShaderStageCreateInfo* p_shader_create_info)
    {
        char*  p_shader_code;
        size_t size;

        ShaderSourceType source_type = {};

        const char* p_extension = p_shder_file_name + std::max<size_t>(strlen(p_shder_file_name) - 4, 0);
        if (strcmp(p_extension, "glsl") == 0)
            source_type = SST_GLSL;
        else if (strcmp(p_extension, "hlsl") == 0)
            source_type = SST_HLSL;
        else
            LOG_ERROR("Can't tell shader type from its extension");

        // append path
        char full_path[1024];
        sprintf_s(full_path, "%s\\%s", getShaderCompilerLibDir().c_str(), p_shder_file_name);

        if (readFile(full_path, &p_shader_code, &size, false))
        {
            VkResult res = vkCompileFromString(device,
                                               source_type,
                                               shader_type,
                                               p_shader_code,
                                               p_shader_entry_point,
                                               p_shader_compiler_params,
                                               p_defines,
                                               p_shader_create_info);
            free(p_shader_code);

            return res;
        }

        return VK_NOT_READY;
    }
} // namespace Bocchi
