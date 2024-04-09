#pragma once
#include "Runtime/Render/RenderSystem.hpp"
#include <Luna/Runtime/String.hpp>
#include <Luna/Window/Window.hpp>

namespace Bocchi
{
    class RenderSystem;
    class RuntimeGlobalContext
    {
        public:
            // create all global system and inititalize these system
            RV StartSystems(const String &config_file_path);

            // destroy all global systems
            void ShutdownSystems();

        public:
            Ref<Window::IWindow> m_window_system;
            Ref<RenderSystem>    m_render_system;
    };
    extern RuntimeGlobalContext g_runtime_global_context;
} //namespace Bocchi