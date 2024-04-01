#pragma once

#include <memory>
#include <string>

namespace bocchi
{
class LogSystem;
class WindowSystem;
class RenderSystem;

class RuntimeGlobalContext
{
    public:
        // create all global system and inititalize these system
        void StartSystems(const std::string &config_file_path);

        // destroy all global systems
        void ShutdownSystems();

    public:
        std::shared_ptr<WindowSystem> m_windows_system;
        std::shared_ptr<RenderSystem> m_render_system;
};

extern RuntimeGlobalContext g_runtime_global_context;
} //namespace bocchi