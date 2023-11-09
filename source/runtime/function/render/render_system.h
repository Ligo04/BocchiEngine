#pragma once

#include <memory>
#include "runtime/function/render/RHI/rhi.h"
#include "runtime/function/render/window_system.h"

#define USE_VK 1

namespace bocchi
{
    class WindowSystem;

    struct RenderSystemInfo
    {
        std::shared_ptr<WindowSystem> window_system;
    };

	class RenderSystem
    {

    public:
        RenderSystem(/* args */) = default;
        ~RenderSystem()=default;

        void initialize(const RenderSystemInfo& render_system_info);
        void tick(float delta_time);
        void clear();

    private:
        std::shared_ptr<Rhi> m_rhi;
    };
}