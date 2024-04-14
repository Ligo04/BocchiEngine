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
#include <Luna/Runtime/String.hpp>
#include <Luna/Runtime/Thread.hpp>
#include <Luna/Runtime/Time.hpp>
#include <Luna/VariantUtils/VariantUtils.hpp>
#include <Luna/Window/MessageBox.hpp>
namespace Bocchi
{
    void BocchiEngine::StartEngine(const String &config_file_path)
    {
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
        log_info("App", "Engine Started!");
    };

    void BocchiEngine::ShutdownEngine() { log_info("App", "Engine Closed!"); }
    void BocchiEngine::SetCurrentDirToProcessPath()
    {
        Path p = get_process_path();
        p.pop_back();
        luassert_always(succeeded(set_current_dir(p.encode().c_str())));
    }

    void BocchiEngine::Run()
    {
        lutry
        {
            while (m_exiting)
            {
                const f64 delta_time = CalculalteDeltaTime();
                luexp(TickOneFrame(delta_time));
            }
        }
        lucatch
        {
            auto _ = Window::message_box(explain(luerr),
                                         "Engine Crashed.",
                                         Window::MessageBoxType::ok,
                                         Window::MessageBoxIcon::error);
        }
    }

    RV BocchiEngine::TickOneFrame(f32 delta_time)
    {
        auto window_system = g_runtime_global_context.m_window_system;
        luassert_always(window_system);
        // window_event
        Window::poll_events();

        if (window_system->is_closed())
        {
            m_exiting = false;
            return ok;
        }
        if (window_system->is_minimized())
        {
            sleep(16);
            return ok;
        }

        LogicalTick(delta_time);
        RendererTick(delta_time);
        CalculateFps(delta_time);
        c8 buf[64];
        snprintf(buf, sizeof(buf), "Bocchi Engine %lld FPS", GetFps());
        lupanic_if_failed(g_runtime_global_context.m_window_system->set_title(buf));
        return ok;
    }

    void      BocchiEngine::LogicalTick(f32 delta_time) {}
    void      BocchiEngine::RendererTick(f32 delta_time) {}

    const f32 BocchiEngine::k_fps_alpha = 1.f / 100;

    void      BocchiEngine::CalculateFps(f32 delta_time)
    {
        m_frame_count++;
        if (m_frame_count == 1)
        {
            m_average_duration = delta_time;
        }
        else
        {
            m_average_duration = m_average_duration * (1 - k_fps_alpha) + delta_time * k_fps_alpha;
        }

        m_fps = static_cast<i32>(1000.0f / m_average_duration);
    }

    f64 BocchiEngine::CalculalteDeltaTime()
    {
        u64 fram_ticks     = get_ticks();
        f64 delta_time     = static_cast<f64>(fram_ticks - m_last_frame_ticks) / get_ticks_per_second() * 1000.0;
        m_last_frame_ticks = fram_ticks;

        return delta_time;
    }
} //namespace Bocchi