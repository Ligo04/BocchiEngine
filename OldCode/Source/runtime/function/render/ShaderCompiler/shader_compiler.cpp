#include "shader_compiler.h"
#include <nvrhi/common/shader-blob.h>

#include "runtime/core/base/macro.h"

namespace bocchi
{
    ShaderCompiler::ShaderCompiler(const nvrhi::DeviceHandle&          renderer_interface,
                                   const std::shared_ptr<IFileSystem>& file_system,
                                   const std::filesystem::path&        base_path) :
        m_device(renderer_interface),
        m_file_system(file_system), m_base_path(base_path)
    {}

    void ShaderCompiler::ClearCache() { m_byte_code_cache_map.clear(); }

    nvrhi::ShaderHandle ShaderCompiler::CreateShader(const char*                     file_name,
                                                     const char*                     entry_name,
                                                     const std::vector<ShaderMacro>* p_defines,
                                                     nvrhi::ShaderType               shader_type)
    {
        nvrhi::ShaderDesc desc = nvrhi::ShaderDesc(shader_type);
        desc.debugName         = file_name;
        return CreateShader(file_name, entry_name, p_defines, desc);
    }

    nvrhi::ShaderHandle ShaderCompiler::CreateShader(const char*                     file_name,
                                                     const char*                     entry_name,
                                                     const std::vector<ShaderMacro>* p_defines,
                                                     nvrhi::ShaderDesc&              shader_desc)
    {
        std::shared_ptr<IBlob> byte_code = GetByteCode(file_name, entry_name);
        if (!byte_code)
        {
            return nullptr;
        }

        std::vector<nvrhi::ShaderConstant> constants;
        if (p_defines)
        {
            for (const ShaderMacro& define : *p_defines)
            {
                constants.push_back(nvrhi::ShaderConstant {define.name.c_str(), define.definition.c_str()});
            }
        }

        nvrhi::ShaderDesc desc_copy = shader_desc;
        desc_copy.entryName         = entry_name;

        const void* permutation_byte_code = nullptr;
        size_t      permutation_size      = 0;
        if (!nvrhi::findPermutationInBlob(byte_code->data(),
                                          byte_code->size(),
                                          constants.data(),
                                          static_cast<uint32_t>(constants.size()),
                                          &permutation_byte_code,
                                          &permutation_size))
        {
            const std::string message = nvrhi::formatShaderNotFoundMessage(
                byte_code->data(), byte_code->size(), constants.data(), static_cast<uint32_t>(constants.size()));

            LOG_ERROR(message);

            return nullptr;
        }

        return m_device->createShader(desc_copy, permutation_byte_code, permutation_size);
    }

    nvrhi::ShaderLibraryHandle ShaderCompiler::CreateShaderLibrary(const char*                     file_name,
                                                                   const std::vector<ShaderMacro>* p_defines)
    {
        std::shared_ptr<IBlob> byte_code = GetByteCode(file_name, nullptr);
        if (!byte_code)
        {
            return nullptr;
        }

                std::vector<nvrhi::ShaderConstant> constants;
        if (p_defines)
        {
            for (const ShaderMacro& define : *p_defines)
            {
                constants.push_back(nvrhi::ShaderConstant {define.name.c_str(), define.definition.c_str()});
            }
        }


        const void* permutation_byte_code = nullptr;
        size_t      permutation_size      = 0;
        if (!nvrhi::findPermutationInBlob(byte_code->data(),
                                          byte_code->size(),
                                          constants.data(),
                                          static_cast<uint32_t>(constants.size()),
                                          &permutation_byte_code,
                                          &permutation_size))
        {
            const std::string message = nvrhi::formatShaderNotFoundMessage(
                byte_code->data(), byte_code->size(), constants.data(), static_cast<uint32_t>(constants.size()));

            LOG_ERROR(message);

            return nullptr;
        }

        return m_device->createShaderLibrary(permutation_byte_code, permutation_size);
    }

    std::shared_ptr<IBlob> ShaderCompiler::GetByteCode(const char* file_name, const char* entry_name)
    {
        if (!entry_name)
        {
            entry_name = "main";
        }

        std::string adjusted_name = file_name;
        {
            // delete hlsl ext name
            size_t pos = adjusted_name.find(".hlsl");
            if (pos!=std::string::npos)
            {
                adjusted_name.erase(pos, 5);
            }

            if (entry_name && strcmp(entry_name, "main"))
            {
                adjusted_name += "_" + std::string(entry_name);
            }
        }



#if USE_VK
        std::filesystem::path shader_file_path = m_base_path / (adjusted_name + ".spirv");
#elif USE_DX12
        std::filesystem::path shader_file_path = m_base_path / (adjusted_name + ".bin");
#endif

        std::shared_ptr<IBlob>& data = m_byte_code_cache_map[shader_file_path.generic_string()];

        if (data)
        {
            return data;
        }

        data = m_file_system->ReadFile(shader_file_path);

        if (!data)
        {
            LOG_ERROR("Couldn't read the binary file for shader %s from %s",
                       file_name,
                      shader_file_path.generic_string().c_str());
        }

        return data;
    }
} // namespace bocchi
