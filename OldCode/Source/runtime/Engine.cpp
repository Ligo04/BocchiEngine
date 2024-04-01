#include "runtime/Engine.h"

#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/render/window_system.h"

namespace bocchi
{
void BocchiEngine::StartEngine(const std::string &config_file_path)
{
    g_runtime_global_context.StartSystems(config_file_path);
}

void BocchiEngine::ShutdownEngine() { g_runtime_global_context.ShutdownSystems(); }

void BocchiEngine::Initialize() {}

void BocchiEngine::clear() {}

void BocchiEngine::run()
{
    std::shared_ptr<WindowSystem> window_system = g_runtime_global_context.m_windows_system;
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
    g_runtime_global_context.m_windows_system->PollEvents();
    g_runtime_global_context.m_windows_system->SetTitle(("Bocchi Engine " + std::to_string(GetFps()) + " FPS").c_str());

    const bool should_window_close = g_runtime_global_context.m_windows_system->ShouldClose();

    return !should_window_close;
}

void        BocchiEngine::LogicalTick(float delta_time) {}

void        BocchiEngine::RendererTick(float delta_time) {}

const float BocchiEngine::kFpsAlpha = 1.f / 100;
void        BocchiEngine::CalculateFps(float delta_time)
{
    m_frame_count++;

    if (m_frame_count == 1)
    {
        m_average_duration = delta_time;
    }
    else
    {
        m_average_duration = m_average_duration * (1 - kFpsAlpha) + delta_time * kFpsAlpha;
    }

    m_fps = static_cast<int>(1.f / m_average_duration);
}

float BocchiEngine::CalculateDeltaTime()
{
    float delta_time;
    {
        using namespace std::chrono;
        steady_clock::time_point tick_time_point;
        auto                     time_span = duration_cast<duration<float>>(tick_time_point - m_last_tick_time_point);

        delta_time                         = time_span.count();
        m_last_tick_time_point             = tick_time_point;
    }
    return delta_time;
}

} //namespace bocchi