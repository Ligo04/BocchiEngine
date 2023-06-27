#include "runtime/core/base/dxc_ptr.h"
#include "runtime/core/base/macro.h"
#include "runtime/core/base/misc.h"
#include "runtime/function/render/ShaderCompiler/dxc_helper.h"
#include <fstream>

#define USE_DXC_SPIRV_FROM_DISK
namespace bocchi
{
    DxcCreateInstanceProc s_dxc_create_func;

    void throwIfFailed(HRESULT hr)
    {
        wchar_t err[256];
        memset(err, 0, 256);
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 255, NULL);
        char   err_a[256];
        size_t return_size;
        wcstombs_s(&return_size, err_a, 255, err, 255);
        LOG_ERROR(err_a);
        throw 1;
    }

    interface IncludeDxc : public IDxcIncludeHandler
    {
        IDxcLibrary* m_p_library;

    public:
        explicit IncludeDxc(IDxcLibrary* p_library) : m_p_library(p_library) {}

        HRESULT QueryInterface(const IID&, void**) override { return S_OK; }

        ULONG AddRef() override { return 0; }

        ULONG Release() override { return 0; }

        HRESULT LoadSource(LPCWSTR p_file_name, IDxcBlob** pp_include_source)
        {
            char full_path[1024];
            sprintf_s<1024>(full_path, ("%s\\%s"), getShaderCompilerLibDir().c_str(), p_file_name);

            LPCVOID p_data;
            size_t  bytes;
            HRESULT hr = readFile(full_path, (char**)&p_data, (size_t*)&bytes, false) ? S_OK : S_FALSE;

            if (hr == E_FAIL)
            {
                // return the failure here instead of crashing on CreateBlobWithEncodingFromPinned
                // to be able to report the error to the output console
                return hr;
            }

            IDxcBlobEncoding* p_source;
            throwIfFailed(
                m_p_library->CreateBlobWithEncodingFromPinned((LPBYTE)p_data, (UINT32)bytes, CP_UTF8, &p_source));
            *pp_include_source = p_source;

            return S_OK;
        }
    };

    bool initDirectXCompiler()
    {
        std::string full_shader_compiler_path = "dxcompiler.dll";
        std::string full_shader_dxil_path     = "dxil.dll";

        HMODULE dxil_module = ::LoadLibraryA(full_shader_dxil_path.c_str());
        HMODULE dxc_module  = ::LoadLibraryA(full_shader_compiler_path.c_str());

        // get DxcCreateInstance func by name
        s_dxc_create_func = (DxcCreateInstanceProc)::GetProcAddress(dxc_module, "DxcCreateInstance");

        return s_dxc_create_func != nullptr;
    }

    bool dxComileToDxo(size_t            hash,
                       const char*       p_src_code,
                       const DefineList* p_defines,
                       const char*       p_entry_point,
                       const char*       p_params,
                       char**            out_spv_data,
                       size_t*           out_spv_size)
    {
        // detect out put bytecode type(DXBC/SRIP-V) and use proper extension
        std::string file_name_out;
        {
            if (auto found = std::string(p_params).find("-spirv"); found == std::string::npos)
            {
                file_name_out = getShaderCompilerCacheDir() + format("\\%p.dxo", hash);
            }
            else
            {
                file_name_out = getShaderCompilerCacheDir() + format("\\%p.spv", hash);
            }
        }

#ifdef USE_DXC_SPIRV_FROM_DISK
        if (readFile(file_name_out.c_str(), out_spv_data, out_spv_size, true) && *out_spv_size > 0)
        {
            return true;
        }
#endif

        // create hlsl file for shader compiler to compile
        std::string   file_name_hlsl = getShaderCompilerCacheDir() + format("\\%p.hlsl", hash);
        std::ofstream ofs(file_name_hlsl, std::ofstream::out);
        ofs << p_src_code;
        ofs.close();

        std::string file_name_pdb = getShaderCompilerCacheDir() + format("\\%p.lld", hash);

        /// get defines;
        ///

        struct DefineElement
        {
            wchar_t name[128];
            wchar_t value[128];
        };

        std::vector<DefineElement> define_elements(p_defines ? p_defines->size() : 0);
        std::vector<DxcDefine>     dxc_defines(p_defines ? p_defines->size() : 0);

        int define_count = 0;
        if (p_defines)
        {
            for (auto it = p_defines->begin(); it != p_defines->end(); ++it)
            {
                auto& define_element = define_elements[define_count];
                swprintf_s<128>(define_element.name, L"%S", it->first.c_str());
                swprintf_s<128>(define_element.value, L"%S", it->first.c_str());

                dxc_defines[define_count].Name  = define_element.name;
                dxc_defines[define_count].Value = define_element.value;
                define_count++;
            }
        }

        // check whether DXCompiler is initailized
        if (s_dxc_create_func == nullptr)
        {
            LOG_ERROR("s_dxc_create_func() is null, have you called InitDirectXCompiler() ?");
        }

        // Init DXC Library
        IDxcLibrary* p_library;
        throwIfFailed(s_dxc_create_func(CLSID_DxcLibrary, IID_PPV_ARGS(&p_library)));

        IDxcBlobEncoding* p_source;
        throwIfFailed(p_library->CreateBlobWithEncodingFromPinned(
            (LPBYTE)p_src_code, (UINT32)strlen(p_src_code), CP_UTF8, &p_source));

        // Init DXC Compiler
        IDxcCompiler2* p_compiler2;
        throwIfFailed(s_dxc_create_func(CLSID_DxcCompiler, IID_PPV_ARGS(&p_compiler2)));

        IncludeDxc           includer(p_library);
        std::vector<LPCWSTR> pp_args;
        // splits params string into an array of strings
        {
            char params[1024];
            strcpy_s<1024>(params, p_params);

            char* next_token;
            char* token = strtok_s(params, " ", &next_token);
            while (token != nullptr)
            {
                wchar_t wide_str[1024];
                swprintf_s<1024>(wide_str, L"%S", token);

                const size_t wide_str2_len = wcslen(wide_str) + 1;
                wchar_t*     wide_str2     = (wchar_t*)malloc(wide_str2_len * sizeof(wchar_t));
                wcscpy_s(wide_str2, wide_str2_len, wide_str);
                pp_args.push_back(wide_str2);

                token = strtok_s(NULL, " ", &next_token);
            }
        }

        wchar_t p_entry_point_w[256];
        swprintf_s<256>(p_entry_point_w, L"%S", p_entry_point);

        IDxcOperationResult* p_result_pre;
        HRESULT              res1 =
            p_compiler2->Preprocess(p_source, L"", NULL, 0, dxc_defines.data(), define_count, &includer, &p_result_pre);
        if (res1 == S_OK)
        {
            DxcPtr<IDxcBlob> p_code1;
            p_result_pre->GetResult(p_code1.getAddressOf());

            std::string preprocess_code = "";
            preprocess_code =
                "// dxc -E" + std::string(p_entry_point) + " " + std::string(p_params) + " " + file_name_hlsl + "\n\n";
            if (p_defines)
            {
                for (auto it = p_defines->begin(); it != p_defines->end(); ++it)
                {
                    preprocess_code += "#define " + it->first + " " + it->second + "\n";
                }
            }

            preprocess_code += std::string((char*)p_code1->GetBufferPointer());
            preprocess_code += "\n";

            saveFile(file_name_hlsl.c_str(), preprocess_code.c_str(), preprocess_code.size(), false);

            IDxcOperationResult* p_op_res;
            HRESULT              res;

            res = p_compiler2->Compile(p_source,
                                       NULL,
                                       p_entry_point_w,
                                       L"",
                                       pp_args.data(),
                                       (UINT32)pp_args.size(),
                                       dxc_defines.data(),
                                       define_count,
                                       &includer,
                                       &p_op_res);

            // clean up allocated memort
            while (!pp_args.empty())
            {
                LPCWSTR p_w_string = pp_args.back();
                pp_args.pop_back();
                free((void*)p_w_string);
            }

            p_source->Release();
            p_library->Release();
            p_compiler2->Release();

            IDxcBlob*         p_result = nullptr;
            IDxcBlobEncoding* p_error  = nullptr;
            if (p_op_res != NULL)
            {
                p_op_res->GetResult(&p_result);
                p_op_res->GetErrorBuffer(&p_error);
                p_op_res->Release();
            }
            if (p_result != NULL && p_result->GetBufferSize() > 0)
            {
                *out_spv_size = p_result->GetBufferSize();
                *out_spv_data = (char*)malloc(*out_spv_size);

                memcpy(*out_spv_data, p_result->GetBufferPointer(), *out_spv_size);

                p_result->Release();

                // make sure pError doesn't leak if it awas allocated
                if (p_error)
                {
                    p_error->Release();
                }

#ifdef USE_DXC_SPIRV_FROM_DISK
                saveFile(file_name_out.c_str(), *out_spv_data, *out_spv_size, true);
#endif
                return true;
            }
            else
            {
                IDxcBlobEncoding* p_error_utf8;
                p_library->GetBlobAsUtf8(p_error, &p_error_utf8);

                LOG_ERROR("*** Error compiling %p.hlsl ***\n", hash)

                std::string file_name_error = getShaderCompilerCacheDir() + format("\\%p.err", hash);
                saveFile(
                    file_name_error.c_str(), p_error_utf8->GetBufferPointer(), p_error_utf8->GetBufferSize(), false);
                std::string err_msg =
                    std::string((char*)p_error_utf8->GetBufferPointer(), p_error_utf8->GetBufferSize());
                LOG_ERROR(err_msg)

                // Make sure pResult doesn't leak if it was allocated
                if (p_result)
                    p_result->Release();

                p_error_utf8->Release();
            }
        }
        return false;
    }

} // namespace Bocchi
