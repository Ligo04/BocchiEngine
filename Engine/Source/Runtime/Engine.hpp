#pragma once
#include "Luna/Runtime/Path.hpp"
#include "Luna/Runtime/Time.hpp"
#include <Luna/Window/Window.hpp>

namespace Bocchi
{
    using namespace Luna;
    class BocchiEngine
    {
            static const float k_fps_alpha;

        public:
            BocchiEngine() {}
            ~BocchiEngine() {}
            void StartEngine(const Path &config_file_path);
            void ShutdownEngine();

            void SetCurrentDirToProcessPath();
            RV   InitEnv();
            void Initialize();
            void Clear();

            bool IsQuit() const { return m_is_quit; }
            void Run();

            bool TickOneFrame(float delta_time);
            int  GetFps() const { return m_fps; }

        public:
            Path  m_project_path;

            i64   m_last_tick_time_point{ get_utc_timestamp() };
            bool  m_is_quit{ false };
            float m_average_duration{ 0.f };
            int   m_frame_count{ 0 };
            int   m_fps{ 0 };
    };
} // namespace Bocchi