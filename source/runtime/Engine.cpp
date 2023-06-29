#include "runtime/Engine.h"

#include "runtime/core/base/macro.h"

#include "runtime/function/global/global_context.h"
#include "runtime/function/render/window_system.h"

namespace bocchi
{
    void BocchiEngine::StartEngine(const std::string& config_file_path)
    {
        g_runtime_global_context.StartSystems(config_file_path);
    }

    void BocchiEngine::ShutdownEngine() { g_runtime_global_context.ShutdownSystems(); }

    void BocchiEngine::Initialize() {}

    void BocchiEngine::clear() {}

    void BocchiEngine::run()
    {
        std::shared_ptr<WindowSystem> window_system = g_runtime_global_context.m_windows_system_;
        ASSERT(window_system);

        while (!window_system->ShouldClose())
        {
            const float delta_time = CalculateDeltaTime();
            TickOneFrame(delta_time);
        }
    }

    bool BocchiEngine::TickOneFrame(float delta_time)
    {
        // first logical tick
        LogicalTick(delta_time);
        CalculateFps(delta_time);
        // then render tick
        RendererTick(delta_time);
        // then swap buffer
        g_runtime_global_context.m_windows_system_->PollEvents();
        g_runtime_global_context.m_windows_system_->SetTitle(
            ("Bocchi Engine " + std::to_string(GetFps()) + " FPS").c_str());

        const bool should_window_close = g_runtime_global_context.m_windows_system_->ShouldClose();

        return !should_window_close;
    }

    void BocchiEngine::LogicalTick(float delta_time) {}

    void BocchiEngine::RendererTick(float delta_time) {}

    const float BocchiEngine::m_s_fps_alpha_ = 1.f / 100;
    void        BocchiEngine::CalculateFps(float delta_time)
    {
        m_frame_count_++;

        if (m_frame_count_ == 1)
        {
            m_average_duration_ = delta_time;
        }
        else
        {
            m_average_duration_ = m_average_duration_ * (1 - m_s_fps_alpha_) + delta_time * m_s_fps_alpha_;
        }

        m_fps_ = static_cast<int>(1.f / m_average_duration_);
    }

    float BocchiEngine::CalculateDeltaTime()
    {
        float delta_time;
        {
            using namespace std::chrono;
            steady_clock::time_point tick_time_point;
            duration<float> time_span = duration_cast<duration<float>>(tick_time_point - m_last_tick_time_point_);

            delta_time             = time_span.count();
            m_last_tick_time_point_ = tick_time_point;
        }
        return delta_time;
    }

} // namespace Bocchi