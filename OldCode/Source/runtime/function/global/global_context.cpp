#include "global_context.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/window_system.h"

namespace bocchi
{
RuntimeGlobalContext g_runtime_global_context;

void                 RuntimeGlobalContext::StartSystems(const std::string &config_file_path)
{
    m_windows_system = std::make_shared<WindowSystem>();
    WindowCreateInfo window_create_info{};
    m_windows_system->initialize(window_create_info);

    m_render_system = std::make_shared<RenderSystem>();
    RenderSystemInfo render_system_info;
    render_system_info.window_system = m_windows_system;

    m_render_system->initialize(render_system_info);
}

void RuntimeGlobalContext::ShutdownSystems()
{
    m_windows_system.reset();
    m_render_system.reset();
}
} //namespace bocchi