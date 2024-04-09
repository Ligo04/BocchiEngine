#include "Engine.hpp"
#include "Runtime/Global/GlobalContext.hpp"
#include <Luna/Asset/Asset.hpp>
#include <Luna/Font/Font.hpp>
#include <Luna/HID/HID.hpp>
#include <Luna/ImGui/ImGui.hpp>
#include <Luna/ObjLoader/ObjLoader.hpp>
#include <Luna/RHI/RHI.hpp>
#include <Luna/Runtime/File.hpp>
#include <Luna/Runtime/Log.hpp>
#include <Luna/Runtime/Runtime.hpp>
#include <Luna/Runtime/Thread.hpp>
#include <Luna/VariantUtils/VariantUtils.hpp>

namespace Bocchi
{
    void BocchiEngine::StartEngine(const String &config_file_path)
    {
        /// m_project_path = config_file_path;
        luassert_always(Luna::init());
        SetCurrentDirToProcessPath();
        // log
        set_log_to_platform_enabled(true);
        set_log_to_platform_verbosity(LogVerbosity::verbose);
        // luna module
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
        // global context
        r = g_runtime_global_context.StartSystems("");
        if (failed(r))
        {
            log_error("App", explain(r.errcode()));
        }
        return;
    };

    void BocchiEngine::SetCurrentDirToProcessPath()
    {
        Path p = get_process_path();
        p.pop_back();
        luassert_always(succeeded(set_current_dir(p.encode().c_str())));
    }

    void BocchiEngine::Run()
    {
        auto window_system = g_runtime_global_context.m_window_system;
        luassert_always(window_system);
        while (!window_system->is_closed())
        {
            const float delta_time = CalculalteDeltaTime();
            TickOneFrame(delta_time);
        }
    }

    bool BocchiEngine::TickOneFrame(f32 delta_time)
    {
        Window::poll_events();
        sleep(16);
        return true;
    }

    f32  BocchiEngine::CalculalteDeltaTime() { return 0.0f; }
    void BocchiEngine::ShutdownEngine() {}
} //namespace Bocchi