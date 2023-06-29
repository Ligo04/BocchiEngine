#include "global_context.h"
#include "runtime/core/log/log_system.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/render_system.h"


namespace bocchi
{
    RuntimeGlobalContext g_runtime_global_context;

    void RuntimeGlobalContext::StartSystems(const std::string& config_file_path)
    {

        m_logger_system_ = std::make_shared<LogSystem>();
        
        m_windows_system_ = std::make_shared<WindowSystem>();
        WindowCreateInfo window_create_info{};
        m_windows_system_->initialize(window_create_info);
        
        m_render_system_ = std::make_shared<RenderSystem>();
        RenderSystemInfo render_system_info;
        render_system_info.window_system = m_windows_system_;

        m_render_system_->initialize(render_system_info);
    }

    void RuntimeGlobalContext::ShutdownSystems()
    {
        m_logger_system_.reset();
        m_windows_system_.reset();
        m_render_system_.reset();

    }
}   // namespace