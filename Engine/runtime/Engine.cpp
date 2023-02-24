#include "runtime/Engine.h"

#include "runtime/core/mico.h"

#include "runtime/function/global/global_context.h"
#include "runtime/render/window_system.h"

namespace Bocchi
{
    void BocchiEngine::startEngine(const std::string& config_file_path)
    {
        g_runtime_global_context.startSystems(config_file_path);
    }

    void BocchiEngine::shutdownEngine()
    {
        g_runtime_global_context.shutdownSystems();
    }

    void BocchiEngine::initialize()
    {
    }

    void BocchiEngine::clear()
    {
    }

    void BocchiEngine::run()
    {
        std::shared_ptr<WindowSystem> window_system = g_runtime_global_context.m_windows_system;
        ASSERT(window_system);

        while (!window_system->shouldClose())
        {
            const float delta_time = calculateDeltaTime();
            tickOneFrame(delta_time);
        }
    }

    bool BocchiEngine::tickOneFrame(float delta_time)
    {
        // first logical tick
        logicalTick(delta_time);
        calculateFPS(delta_time);


        // then render tick
        rendererTick(delta_time);

        g_runtime_global_context.m_windows_system->pollEvents();

        g_runtime_global_context.m_windows_system->setTitle(
            std::string("Bocchi Engine " + std::to_string(getFPS()) + " FPS").c_str());
        
        const bool should_window_close = g_runtime_global_context.m_windows_system->shouldClose();

        return !should_window_close;
    }

    void BocchiEngine::logicalTick(float delta_time)
    {
    }

    void BocchiEngine::rendererTick(float delta_time)
    {
    }

    const float BocchiEngine::s_fps_alpha = 1.f / 100;
    void BocchiEngine::calculateFPS(float delta_time)
    {
        m_frame_count++;

        if (m_frame_count == 1)
        {
            m_average_duration = delta_time;
        }
        else
        {
            m_average_duration = m_average_duration * (1 - s_fps_alpha) + delta_time * s_fps_alpha;
        }

        m_fps = static_cast<int>(1.f / m_average_duration);
    }

    float BocchiEngine::calculateDeltaTime()
    {
        float delta_time;
        {
            using namespace std::chrono;
            steady_clock::time_point tick_time_point;
            duration<float>          time_span =
                duration_cast<duration<float>>(tick_time_point - m_last_tick_time_point);

            delta_time             = time_span.count();
            m_last_tick_time_point = tick_time_point;
        }
        return delta_time;
    }

}   // namespace Bocchi