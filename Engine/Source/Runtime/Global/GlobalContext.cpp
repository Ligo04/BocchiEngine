#include "GlobalContext.hpp"
#include <Luna/Runtime/Ref.hpp>

namespace Bocchi
{
    RuntimeGlobalContext g_runtime_global_context;

    RV                   RuntimeGlobalContext::StartSystems(const String &config_file_path)
    {
        Luna::register_boxed_type<RenderSystem>();
        lutry
        {
            luset(m_window_system,
                  Window::new_window("Bocchi Engine",
                                     Window::WindowDisplaySettings::as_windowed(),
                                     Window::WindowCreationFlag::resizable));
            m_window_system->get_close_event().add_handler([](Window::IWindow *window) { window->close(); });
            m_window_system->get_framebuffer_resize_event().add_handler(
                [this](Window::IWindow *window, u32 width, u32 height) {
                    //TODO: Windows Resize Event
                });
        }
        lucatchret;
        RenderSystemInfo render_system_info;
        render_system_info.window = m_window_system;
        m_render_system           = new_object<RenderSystem>();
        m_render_system->Initialize(render_system_info);
        return ok;
    }

    void RuntimeGlobalContext::ShutdownSystems() {}
} //namespace Bocchi