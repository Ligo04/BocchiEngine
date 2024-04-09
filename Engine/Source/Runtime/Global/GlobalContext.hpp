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
            void StartSystems(const Luna::String &config_file_path);

            // destroy all global systems
            void ShutdownSystems();

        public:
            Luna::Ref<Luna::Window::IWindow> m_window_system;
            Luna::Ref<RenderSystem>          m_render_system;
    };
    extern RuntimeGlobalContext g_runtime_global_context;
} //namespace Bocchi