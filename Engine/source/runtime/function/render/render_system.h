#pragma once

#include "runtime/function/render/RHI/vulkan/rhi_vk.h"

#include "runtime/function/render/window_system.h"

namespace Bocchi
{
    class RHI;
    class WindowSystem;

    struct RenderSystemInfo
    {
        std::shared_ptr<WindowSystem> window_system;
    };

	class RenderSystem
    {

    public:
        RenderSystem(/* args */) = default;
        ~RenderSystem();

        void initialize(RenderSystemInfo render_system_info);
        void tick(float delta_time);
        void clear();

    private:
        std::shared_ptr<RHI> m_rhi;
    };
}