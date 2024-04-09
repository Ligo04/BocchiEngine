#include "Engine.hpp"
#include <Luna/Asset/Asset.hpp>
#include <Luna/Font/Font.hpp>
#include <Luna/HID/HID.hpp>
#include <Luna/ImGui/ImGui.hpp>
#include <Luna/ObjLoader/ObjLoader.hpp>
#include <Luna/RHI/RHI.hpp>
#include <Luna/Runtime/File.hpp>
#include <Luna/Runtime/Log.hpp>
#include <Luna/Runtime/Runtime.hpp>
#include <Luna/VariantUtils/VariantUtils.hpp>

namespace Bocchi
{
    void BocchiEngine::StartEngine(const Path &config_file_path)
    {
        m_project_path = config_file_path;
        return;
    };

    void BocchiEngine::Initialize()
    {
        luassert_always(Luna::init());
        SetCurrentDirToProcessPath();
        // log
        set_log_to_platform_enabled(true);
        set_log_to_platform_verbosity(LogVerbosity::verbose);
        lupanic_if_failed(add_modules({ module_variant_utils(),
                                        module_hid(),
                                        module_window(),
                                        module_rhi(),
                                        module_font(),
                                        module_imgui(),
                                        module_asset(),
                                        module_obj_loader() }));
        auto r = init_modules();
        if (failed(r))
        {
            log_error("App", explain(r.errcode()));
        }
        return;
    }

    void BocchiEngine::SetCurrentDirToProcessPath()
    {
        Path p = get_process_path();
        p.pop_back();
        luassert_always(succeeded(set_current_dir(p.encode().c_str())));
    }

    void BocchiEngine::Run() {}

    void BocchiEngine::Clear() { Luna::close(); }

    void BocchiEngine::ShutdownEngine() {}
} //namespace Bocchi