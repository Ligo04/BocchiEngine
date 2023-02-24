#include "window_system.h"


namespace Bocchi
{
    WindowSystem::~WindowSystem()
    {
        glfwDestroyWindow(m_pwindow);
        glfwTerminate();
    }

    void WindowSystem::initialize(WindowCreateInfo info)
    {
        if ( !glfwInit() )
        {
            return;
        }

        m_width  = info.width;
        m_height = info.height;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_pwindow = glfwCreateWindow(info.width, info.height, info.title, nullptr, nullptr);

        if ( !m_pwindow )
        {
            glfwTerminate();

            return;
        }

        // setup input callbacks
        glfwSetWindowUserPointer(m_pwindow, this);
        glfwSetKeyCallback(m_pwindow, keyCallback);
        glfwSetCharCallback(m_pwindow, charCallback);
        glfwSetCharModsCallback(m_pwindow, charModsCallback);
        glfwSetMouseButtonCallback(m_pwindow, mouseButtonCallback);
        glfwSetCursorPosCallback(m_pwindow, cursorPosCallback);
        glfwSetCursorEnterCallback(m_pwindow, cursorEnterCallback);
        glfwSetScrollCallback(m_pwindow, scrollCallback);
        glfwSetDropCallback(m_pwindow, dropCallback);
        glfwSetWindowSizeCallback(m_pwindow, windowSizeCallback);
        glfwSetWindowCloseCallback(m_pwindow, windowCloseCallback);

        glfwSetInputMode(m_pwindow, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
    }

    void WindowSystem::pollEvents() const
    {
        glfwPollEvents();
    }

    bool WindowSystem::shouldClose() const
    {
        return glfwWindowShouldClose(m_pwindow);
    }

    void WindowSystem::setTitle(const char* title)
    {
        glfwSetWindowTitle(m_pwindow, title);
    }

    GLFWwindow* WindowSystem::getWindow() const
    {
        return m_pwindow;
    }

    std::array<int, 2> WindowSystem::getWindowsSize() const
    {
        return std::array<int, 2>({m_width, m_height});
    }

    void WindowSystem::setFocusMode(bool mode)
    {
        m_is_focus_mode = mode;
        glfwSetInputMode(
            m_pwindow, GLFW_CURSOR, m_is_focus_mode ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
}   // namespace Bocchi