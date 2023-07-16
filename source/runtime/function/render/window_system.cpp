#include "window_system.h"


namespace bocchi
{
    WindowSystem::~WindowSystem()
    {
        glfwDestroyWindow(m_pwindow_);
        glfwTerminate();
    }

    void WindowSystem::initialize(const WindowCreateInfo& info)
    {
        if (!glfwInit())
        {
            return;
        }

        m_width_  = info.width;
        m_height_ = info.height;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_pwindow_ = glfwCreateWindow(info.width, info.height, info.title, nullptr, nullptr);

        if (!m_pwindow_)
        {
            glfwTerminate();
            return;
        }

        // setup input callbacks
        glfwSetWindowUserPointer(m_pwindow_, this);
        glfwSetKeyCallback(m_pwindow_, KeyCallback);
        glfwSetCharCallback(m_pwindow_, CharCallback);
        glfwSetCharModsCallback(m_pwindow_, CharModsCallback);
        glfwSetMouseButtonCallback(m_pwindow_, MouseButtonCallback);
        glfwSetCursorPosCallback(m_pwindow_, CursorPosCallback);
        glfwSetCursorEnterCallback(m_pwindow_, CursorEnterCallback);
        glfwSetScrollCallback(m_pwindow_, ScrollCallback);
        glfwSetDropCallback(m_pwindow_, DropCallback);
        glfwSetWindowSizeCallback(m_pwindow_, WindowSizeCallback);
        glfwSetWindowCloseCallback(m_pwindow_, WindowCloseCallback);

        glfwSetInputMode(m_pwindow_, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
    }

    void WindowSystem::PollEvents()
    {
        glfwPollEvents();
    }

    bool WindowSystem::ShouldClose() const
    {
        return glfwWindowShouldClose(m_pwindow_);
    }

    void WindowSystem::SetTitle(const char* title) const
    {
        glfwSetWindowTitle(m_pwindow_, title);
    }

    GLFWwindow* WindowSystem::GetWindow() const
    {
        return m_pwindow_;
    }

    std::array<int, 2> WindowSystem::GetWindowsSize() const
    {
        return std::array<int, 2>({m_width_, m_height_});
    }

    bool WindowSystem::GetFocusMode() const { return m_is_focus_mode_; }

    void WindowSystem::SetFocusMode(bool mode)
    {
        m_is_focus_mode_ = mode;
        glfwSetInputMode(
            m_pwindow_, GLFW_CURSOR, m_is_focus_mode_ ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
}   // namespace Bocchi