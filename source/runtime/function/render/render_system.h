#pragma once

#include <memory>
#include <nvrhi/nvrhi.h>
#include "runtime/function/render/window_system.h"

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
        ~RenderSystem();

        void initialize(const RenderSystemInfo& render_system_info);
        void tick(float delta_time);
        void clear();

    private:
        
    };
}