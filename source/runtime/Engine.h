#pragma once
#include <chrono>
#include <string>

namespace bocchi
{
    class BocchiEngine
    {
        static const float kFpsAlpha;

    public:
        void StartEngine(const std::string& config_file_path);
        void ShutdownEngine();

        void Initialize();
        void clear();

        bool IsQuit() const { return m_is_quit; }

        void run();
        bool TickOneFrame(float delta_time);

        int GetFps() const { return m_fps; }

    protected:
        void LogicalTick(float delta_time);
        void RendererTick(float delta_time);

        void CalculateFps(float delta_time);

        float CalculateDeltaTime();

        bool                                  m_is_quit {false};
        std::chrono::steady_clock::time_point m_last_tick_time_point {std::chrono::steady_clock::now()};

        float m_average_duration {0.f};
        int   m_frame_count {0};
        int   m_fps {0};
    };
} // namespace Bocchi