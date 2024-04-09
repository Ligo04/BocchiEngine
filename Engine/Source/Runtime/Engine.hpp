#pragma once
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
            void StartEngine(const String &config_file_path);
            void ShutdownEngine();

            void SetCurrentDirToProcessPath();
            void InitEnv();

            bool IsQuit() const { return m_is_quit; }
            void Run();

            bool TickOneFrame(f32 delta_time);
            f32  CalculalteDeltaTime();
            int  GetFps() const { return m_fps; }

        public:
            //Path  m_project_path{};

            i64  m_last_tick_time_point{ 0 };
            bool m_is_quit{ false };
            f32  m_average_duration{ 0.f };
            i32  m_frame_count{ 0 };
            i32  m_fps{ 0 };
    };
} // namespace Bocchi