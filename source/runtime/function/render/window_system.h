#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include<array>
#include<functional>
#include<vector>


namespace bocchi
{
    struct WindowCreateInfo
    {
        int         width{1280};
        int         height{720};
        const char* title{"Bocchi Engine"};
        bool        is_fullscreen{false};
    };

    class WindowSystem
    {
    public:
        WindowSystem(/* args */) = default;
        ~WindowSystem();

        void               initialize(const WindowCreateInfo& info);

        static void        PollEvents();
        bool               ShouldClose() const;
        void               SetTitle(const char* title) const;
        GLFWwindow*        GetWindow() const;
        std::array<int, 2> GetWindowsSize() const;

        typedef std::function<void()>                   OnResetFunc;
        typedef std::function<void(int, int, int, int)> OnKeyFunc;
        typedef std::function<void(unsigned int)>       OnCharFunc;
        typedef std::function<void(int, unsigned int)>  OnCharModsFunc;
        typedef std::function<void(int, int, int)>      OnMouseButtonFunc;
        typedef std::function<void(double, double)>     OnCursorPosFunc;
        typedef std::function<void(int)>                OnCursorEnterFunc;
        typedef std::function<void(double, double)>     OnScrollFunc;
        typedef std::function<void(int, const char**)>  OnDropFunc;
        typedef std::function<void(int, int)>           OnWindowSizeFunc;
        typedef std::function<void()>                   OnWindowCloseFunc;

        void RegisterOnResetFunc(const OnResetFunc& func) { m_on_reset_func_.push_back(func); }
        void RegisterOnKeyFunc(const OnKeyFunc& func) { m_on_key_func_.push_back(func); }
        void RegisterOnCharFunc(const OnCharFunc& func) { m_on_char_func_.push_back(func); }
        void RegisterOnCharModsFunc(const OnCharModsFunc& func) { m_on_char_mods_func_.push_back(func); }
        void RegisterOnMouseButtonFunc(const OnMouseButtonFunc& func)
        {
            m_on_mouse_button_func_.push_back(func);
        }
        void RegisterOnCursorPosFunc(const OnCursorPosFunc& func) { m_on_cursor_pos_func_.push_back(func); }
        void RegisterOnCursorEnterFunc(const OnCursorEnterFunc& func)
        {
            m_on_cursor_enter_func_.push_back(func);
        }
        void RegisterOnScrollFunc(const OnScrollFunc& func) { m_on_scroll_func_.push_back(func); }
        void RegisterOnDropFunc(const OnDropFunc& func) { m_on_drop_func_.push_back(func); }
        void RegisterOnWindowSizeFunc(const OnWindowSizeFunc& func) { m_on_window_size_func_.push_back(func); }
        void RegisterOnWindowCloseFunc(const OnWindowCloseFunc& func)
        {
            m_on_window_close_func_.push_back(func);
        }

        [[nodiscard]] bool IsMouseButtonDown(int button) const
        {
            if ( button < GLFW_MOUSE_BUTTON_1 || button > GLFW_MOUSE_BUTTON_LAST )
            {
                return false;
            }
            return glfwGetMouseButton(m_pwindow_, button) == GLFW_PRESS;
        }

        bool GetFocusMode() const;
        void SetFocusMode(bool mode);

    protected:   
        // window event callbacks
        static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
        {
	        if ( auto app = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window)) )
            {
                app->OnKey(key, scancode, action, mods);
            }
        }
        static void CharCallback(GLFWwindow* window, unsigned int codepoint)
        {
	        if ( auto app = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window)) )
            {
                app->OnChar(codepoint);
            }
        }
        static void CharModsCallback(GLFWwindow* window, unsigned int codepoint, int mods)
        {
	        if ( auto app = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window)) )
            {
                app->OnCharMods(codepoint, mods);
            }
        }
        static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
        {
	        if ( auto app = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window)) )
            {
                app->OnMouseButton(button, action, mods);
            }
        }
        static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
        {
	        if (  auto app = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window)) )
            {
                app->OnCursorPos(xpos, ypos);
            }
        }
        static void CursorEnterCallback(GLFWwindow* window, int entered)
        {
	        if ( auto app = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window)) )
            {
                app->OnCursorEnter(entered);
            }
        }
        static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
        {
	        if ( auto app = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window)) )
            {
                app->OnScroll(xoffset, yoffset);
            }
        }
        static void DropCallback(GLFWwindow* window, int count, const char** paths)
        {
	        if ( auto app = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window)) )
            {
                app->OnDrop(count, paths);
            }
        }
        static void WindowSizeCallback(GLFWwindow* window, int width, int height)
        {
	        if ( auto app = static_cast<WindowSystem*>(glfwGetWindowUserPointer(window)) )
            {
                app->m_width_  = width;
                app->m_height_ = height;
            }
        }
        static void WindowCloseCallback(GLFWwindow* window)
        {
            glfwSetWindowShouldClose(window, true);
        }

        void OnReset()
        {
            for ( auto& func : m_on_reset_func_ ) func();
        }
        void OnKey(int key, int scancode, int action, int mods)
        {
            for ( auto& func : m_on_key_func_ ) func(key, scancode, action, mods);
        }
        void OnChar(unsigned int codepoint)
        {
            for ( auto& func : m_on_char_func_ ) func(codepoint);
        }
        void OnCharMods(int codepoint, unsigned int mods)
        {
            for ( auto& func : m_on_char_mods_func_ ) func(codepoint, mods);
        }
        void OnMouseButton(int button, int action, int mods)
        {
            for ( auto& func : m_on_mouse_button_func_ ) func(button, action, mods);
        }
        void OnCursorPos(double xpos, double ypos)
        {
            for ( auto& func : m_on_cursor_pos_func_ ) func(xpos, ypos);
        }
        void OnCursorEnter(int entered)
        {
            for ( auto& func : m_on_cursor_enter_func_ ) func(entered);
        }
        void OnScroll(double xoffset, double yoffset)
        {
            for ( auto& func : m_on_scroll_func_ ) func(xoffset, yoffset);
        }
        void OnDrop(int count, const char** paths)
        {
            for ( auto& func : m_on_drop_func_ ) func(count, paths);
        }
        void OnWindowSize(int width, int height)
        {
            for ( auto& func : m_on_window_size_func_ ) func(width, height);
        }

    private:
        GLFWwindow* m_pwindow_{nullptr};

        int m_width_{0};
        int m_height_{0};

        bool m_is_focus_mode_{false};

        std::vector<OnResetFunc>       m_on_reset_func_;
        std::vector<OnKeyFunc>         m_on_key_func_;
        std::vector<OnCharFunc>        m_on_char_func_;
        std::vector<OnCharModsFunc>    m_on_char_mods_func_;
        std::vector<OnMouseButtonFunc> m_on_mouse_button_func_;
        std::vector<OnCursorPosFunc>   m_on_cursor_pos_func_;
        std::vector<OnCursorEnterFunc> m_on_cursor_enter_func_;
        std::vector<OnScrollFunc>      m_on_scroll_func_;
        std::vector<OnDropFunc>        m_on_drop_func_;
        std::vector<OnWindowSizeFunc>  m_on_window_size_func_;
        std::vector<OnWindowCloseFunc> m_on_window_close_func_;
    };
}

