#include "window_system.h"

namespace bocchi
{
    WindowSystem::WindowSystem() {}

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

    void WindowSystem::PollEvents() { glfwPollEvents(); }

    bool WindowSystem::ShouldClose() const { return glfwWindowShouldClose(m_pwindow_); }

    void WindowSystem::SetTitle(const char* title) const { glfwSetWindowTitle(m_pwindow_, title); }

    GLFWwindow* WindowSystem::GetWindow() const { return m_pwindow_; }

    std::array<int, 2> WindowSystem::GetWindowsSize() const { return std::array<int, 2>({m_width_, m_height_}); }

    void WindowSystem::RegisterOnResetFunc(const OnResetFunc& func) { m_on_reset_func_.push_back(func); }

    void WindowSystem::RegisterOnKeyFunc(const OnKeyFunc& func) { m_on_key_func_.push_back(func); }

    void WindowSystem::RegisterOnCharFunc(const OnCharFunc& func) { m_on_char_func_.push_back(func); }

    void WindowSystem::RegisterOnCharModsFunc(const OnCharModsFunc& func) { m_on_char_mods_func_.push_back(func); }

    void WindowSystem::RegisterOnMouseButtonFunc(const OnMouseButtonFunc& func)
    {
        m_on_mouse_button_func_.push_back(func);
    }

    void WindowSystem::RegisterOnCursorPosFunc(const OnCursorPosFunc& func) { m_on_cursor_pos_func_.push_back(func); }

    void WindowSystem::RegisterOnCursorEnterFunc(const OnCursorEnterFunc& func)
    {
        m_on_cursor_enter_func_.push_back(func);
    }

    void WindowSystem::RegisterOnScrollFunc(const OnScrollFunc& func) { m_on_scroll_func_.push_back(func); }

    void WindowSystem::RegisterOnDropFunc(const OnDropFunc& func) { m_on_drop_func_.push_back(func); }

    void WindowSystem::RegisterOnWindowSizeFunc(const OnWindowSizeFunc& func)
    {
        m_on_window_size_func_.push_back(func);
    }

    void WindowSystem::RegisterOnWindowCloseFunc(const OnWindowCloseFunc& func)
    {
        m_on_window_close_func_.push_back(func);
    }

    bool WindowSystem::IsMouseButtonDown(int button) const
    {
        if (button < GLFW_MOUSE_BUTTON_1 || button > GLFW_MOUSE_BUTTON_LAST)
        {
            return false;
        }
        return glfwGetMouseButton(m_pwindow_, button) == GLFW_PRESS;
    }

    void WindowSystem::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if (auto app = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window)))
        {
            app->OnKey(key, scancode, action, mods);
        }
    }

    void WindowSystem::CharCallback(GLFWwindow* window, unsigned codepoint)
    {
        if (auto app = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window)))
        {
            app->OnChar(codepoint);
        }
    }

    void WindowSystem::CharModsCallback(GLFWwindow* window, unsigned codepoint, int mods)
    {
        if (auto app = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window)))
        {
            app->OnCharMods(codepoint, mods);
        }
    }

    void WindowSystem::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        if (auto app = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window)))
        {
            app->OnMouseButton(button, action, mods);
        }
    }

    void WindowSystem::CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
    {
        if (auto app = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window)))
        {
            app->OnCursorPos(xpos, ypos);
        }
    }

    void WindowSystem::CursorEnterCallback(GLFWwindow* window, int entered)
    {
        if (auto app = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window)))
        {
            app->OnCursorEnter(entered);
        }
    }

    void WindowSystem::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
    {
        if (auto app = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window)))
        {
            app->OnScroll(xoffset, yoffset);
        }
    }

    void WindowSystem::DropCallback(GLFWwindow* window, int count, const char** paths)
    {
        if (auto app = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window)))
        {
            app->OnDrop(count, paths);
        }
    }

    void WindowSystem::WindowSizeCallback(GLFWwindow* window, int width, int height)
    {
        if (auto app = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window)))
        {
            app->m_width_  = width;
            app->m_height_ = height;
        }
    }

    void WindowSystem::WindowCloseCallback(GLFWwindow* window) { glfwSetWindowShouldClose(window, true); }

    void WindowSystem::OnReset()
    {
        for (auto& func : m_on_reset_func_)
            func();
    }

    void WindowSystem::OnKey(int key, int scancode, int action, int mods)
    {
        for (auto& func : m_on_key_func_)
            func(key, scancode, action, mods);
    }

    void WindowSystem::OnChar(unsigned codepoint)
    {
        for (auto& func : m_on_char_func_)
            func(codepoint);
    }

    void WindowSystem::OnCharMods(int codepoint, unsigned mods)
    {
        for (auto& func : m_on_char_mods_func_)
            func(codepoint, mods);
    }

    void WindowSystem::OnMouseButton(int button, int action, int mods)
    {
        for (auto& func : m_on_mouse_button_func_)
            func(button, action, mods);
    }

    void WindowSystem::OnCursorPos(double xpos, double ypos)
    {
        for (auto& func : m_on_cursor_pos_func_)
            func(xpos, ypos);
    }

    void WindowSystem::OnCursorEnter(int entered)
    {
        for (auto& func : m_on_cursor_enter_func_)
            func(entered);
    }

    void WindowSystem::OnScroll(double xoffset, double yoffset)
    {
        for (auto& func : m_on_scroll_func_)
            func(xoffset, yoffset);
    }

    void WindowSystem::OnDrop(int count, const char** paths)
    {
        for (auto& func : m_on_drop_func_)
            func(count, paths);
    }

    void WindowSystem::OnWindowSize(int width, int height)
    {
        for (auto& func : m_on_window_size_func_)
            func(width, height);
    }

    bool WindowSystem::GetFocusMode() const { return m_is_focus_mode_; }

    void WindowSystem::SetFocusMode(bool mode)
    {
        m_is_focus_mode_ = mode;
        glfwSetInputMode(m_pwindow_, GLFW_CURSOR, m_is_focus_mode_ ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
} // namespace bocchi