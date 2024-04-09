#pragma once

#include "Runtime/Render/RHIContent.hpp"
#include <Luna/RHI/RHI.hpp>
#include <Luna/Runtime/Ref.hpp>
#include <Luna/Window/Window.hpp>

namespace Bocchi
{
    struct RenderSystemInfo
    {
            Luna::Ref<Luna::Window::IWindow> window{ nullptr };
    };

    class RenderSystem
    {
        public:
            RenderSystem()  = default;
            ~RenderSystem() = default;

            void Initialize(const RenderSystemInfo &render_system_info);
            void Tick(float delta_time);
            void Clear();

        private:
            Luna::Ref<RHIContent> m_rhi;
    };
} //namespace Bocchi