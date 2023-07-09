#include "render_system.h"
#include "nvrhi/nvrhi.h"
#include "runtime/function/render/RHI/vulkan_rhi.h"

void bocchi::RenderSystem::initialize(const RenderSystemInfo& render_system_info)
{

    // current rhi:vulkan
    nvrhi::GraphicsAPI api = nvrhi::GraphicsAPI::VULKAN;

	RhiInitInfo rhi_init_info;
    rhi_init_info.back_buffer_width  = render_system_info.window_system->GetWindowsSize()[0];
    rhi_init_info.back_buffer_height = render_system_info.window_system->GetWindowsSize()[1];
    m_rhi_                           = std::shared_ptr<Rhi>(Rhi::Create(api));
    m_rhi_->Initialize(rhi_init_info, render_system_info.window_system->GetWindow());
    

}

void bocchi::RenderSystem::tick(float delta_time)
{
    
}

void bocchi::RenderSystem::clear()
{

}
