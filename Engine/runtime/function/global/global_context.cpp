#include "global_context.h"
#include "runtime/render/window_system.h"


namespace Bocchi
{
    RuntimeGlobalContext g_runtime_global_context;

    void RuntimeGlobalContext::startSystems(const std::string& config_file_path)
    {
        m_windows_system = std::make_shared<WindowSystem>();
        WindowCreateInfo window_create_info;
        m_windows_system->initialize(window_create_info);
    }

    void RuntimeGlobalContext::shutdownSystems()
    {
        m_windows_system.reset();
    }
}   // namespace