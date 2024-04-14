#pragma once
#include <Luna/Window/Window.hpp>

namespace Bocchi
{
    using namespace Luna;
    class BocchiEngine
    {
        public:
            lustruct("BocchiEngine", "{6D86DD3B-05FF-FFCD-9AAE-15FADB6B3F42}");

        public:
            BocchiEngine() {}
            ~BocchiEngine() {}
            void StartEngine(const String &config_file_path);
            void ShutdownEngine();

            void SetCurrentDirToProcessPath();
            void InitEnv();

            bool IsQuit() const { return m_is_quit; }
            void Run();

            RV   TickOneFrame(f32 delta_time);
            i64  GetFps() const { return m_fps; }

        protected:
            void             LogicalTick(f32 delta_time);
            void             RendererTick(f32 delta_time);

            f64              CalculalteDeltaTime();
            void             CalculateFps(f32 delta_time);

            bool             m_exiting{ true };
            bool             m_is_quit{ false };

            u64              m_last_frame_ticks{ 0 };
            f64              m_average_duration{ 0.f };
            f64              m_frame_count{ 0 };
            i64              m_fps{ 0 };

            static const f32 k_fps_alpha;
    };
} // namespace Bocchi