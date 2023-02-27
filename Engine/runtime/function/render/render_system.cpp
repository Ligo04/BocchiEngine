#include "render_system.h"

Bocchi::RenderSystem::~RenderSystem()
{
}

void Bocchi::RenderSystem::initialize(RenderSystemInfo render_system_info)
{
    RHIInitInfo rhi_init_info{};
    rhi_init_info.window_system = render_system_info.window_system;

    m_rhi = std::make_shared<VulkanRHI>();
    m_rhi->initialize(rhi_init_info);
}

void Bocchi::RenderSystem::tick(float delta_time)
{
}

void Bocchi::RenderSystem::clear()
{
    if (m_rhi)
    {
        m_rhi->clear();
    }
    m_rhi.reset();
}
