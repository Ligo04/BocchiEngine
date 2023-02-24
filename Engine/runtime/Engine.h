#pragma once
#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <unordered_set>

namespace Bocchi
{
    class BocchiEngine
    {
        static const float s_fps_alpha;
    public:
        void startEngine(const std::string& config_file_path);
        void shutdownEngine();

        void initialize();
        void clear();

        bool isQuit() const { return m_isQuit; };

        void run();
        bool tickOneFrame(float delta_time);

        int getFPS() const { return m_fps; }

    protected:
        void logicalTick(float delta_time);
        void rendererTick(float delta_time);

        void calculateFPS(float delta_time);


        float calculateDeltaTime();

    protected:
        bool                                  m_isQuit{false};
        std::chrono::steady_clock::time_point m_last_tick_time_point{std::chrono::steady_clock::now()};

        float m_average_duration{0.f};
        int   m_frame_count{0};
        int   m_fps{0};
    };
}   // namespace Bocchi