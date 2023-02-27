#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>

#include <functional>
#include <memory>
#include <vector>

#include "rhi_struct.h"
namespace Bocchi
{
    class WindowSystem;

    struct RHIInitInfo
    {
        std::shared_ptr<WindowSystem> window_system;
    };

    class RHI
    {
    public:
        virtual ~RHI() = 0;

        virtual void initialize(RHIInitInfo initialize_info) = 0;
        virtual void prepareContext()                        = 0;

        virtual void clear()                                 = 0;

    private:
    };

    inline RHI::~RHI() = default;
}   // namespace Bocchi
