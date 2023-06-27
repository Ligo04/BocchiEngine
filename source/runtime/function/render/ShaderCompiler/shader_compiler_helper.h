#pragma once
#include "runtime/function/render/ShaderCompiler/shader_compiler.h"
#include <vulkan/vulkan.h>


namespace bocchi
{
    enum ShaderSourceType
    {
        SST_HLSL,
        SST_GLSL
    };

    void createShaderCache();
    void destoryShaderCache();

    // does as the funciton name
    VkResult vkCompileFromString(VkDevice                         device,
                                 ShaderSourceType                 source_type,
                                 const VkShaderStageFlagBits      shader_type,
                                 const char*                      p_shader_code,
                                 const char*                      p_shader_entry_point,
                                 const char*                      shader_compiler_params,
                                 const DefineList*                p_defines,
                                 VkPipelineShaderStageCreateInfo* p_shader_create_info);

    VkResult vkCompileFromFile(VkDevice                         device,
                               const VkShaderStageFlagBits      shader_type,
                               const char*                      p_shder_file_name,
                               const char*                      p_shader_entry_point,
                               const char*                      p_shader_compiler_params,
                               const DefineList*                p_defines,
                               VkPipelineShaderStageCreateInfo* p_shader_create_info);
} // namespace Bocchi
