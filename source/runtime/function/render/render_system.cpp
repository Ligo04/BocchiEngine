#include "render_system.h"
#include "runtime/function/render/RHI/vulkan_rhi.h"

bocchi::RenderSystem::~RenderSystem()
{
}

void bocchi::RenderSystem::initialize(const RenderSystemInfo& render_system_info)
{
#if USE_VK
    nvrhi::GraphicsAPI api = nvrhi::GraphicsAPI::VULKAN;
#elif USE_DX12
    nvrhi::GraphicsAPI api = nvrhi::GraphicsAPI::D3D12;
#elif USE_DX11
    nvrhi::GraphicsAPI api = nvrhi::GraphicsAPI::D3D11;
#else
#error "No Graphics API defined"
#endif


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
