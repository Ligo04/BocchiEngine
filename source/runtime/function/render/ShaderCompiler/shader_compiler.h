#pragma once
#include "nvrhi/nvrhi.h"
#include "runtime/function/platform/file_system.h"

namespace bocchi
{
    class IBlob;
    class IFileSystem;


    struct ShaderMacro
    {
        std::string name;
        std::string definition;

        ShaderMacro(const std::string& name, const std::string& definition) : name(name), definition(definition) {}
    };

    class ShaderCompiler
    {
    public:
        ShaderCompiler(const nvrhi::DeviceHandle&          renderer_interface,
                       const std::shared_ptr<IFileSystem>& file_system,
                       const std::filesystem::path&        base_path);

        void ClearCache();

        nvrhi::ShaderHandle CreateShader(const char*                     file_name,
                                         const char*                     entry_name,
                                         const std::vector<ShaderMacro>* p_defines,
                                         nvrhi::ShaderType               shader_type);

    	nvrhi::ShaderHandle CreateShader(const char*                     file_name,
                                         const char*                     entry_name,
                                         const std::vector<ShaderMacro>* p_defines,
                                         nvrhi::ShaderDesc&              shader_desc);

        nvrhi::ShaderLibraryHandle CreateShaderLibrary(const char*                     file_name,
                                                       const std::vector<ShaderMacro>* p_defines);

        std::shared_ptr<IBlob> GetByteCode(const char* file_name, const char* entry_name);

    private:
        nvrhi::DeviceHandle m_device_;
        std::unordered_map<std::string, std::shared_ptr<IBlob>> m_byte_code_cache_map_;
        std::shared_ptr<IFileSystem>                            m_file_system_;
        std::filesystem::path                                   m_base_path_;
    };
}
