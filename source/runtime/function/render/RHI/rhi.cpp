#include "rhi.h"

#include <runtime/core/base/macro.h>

//
template<>
struct fmt::formatter<nvrhi::GraphicsAPI> : formatter<string_view>
{
    // parse is inherited from formatter<string_view>.

    auto format(const nvrhi::GraphicsAPI c, format_context& ctx) const
    {
        string_view name = "unknown";
        switch (c)
        {
            case nvrhi::GraphicsAPI::D3D11:
                name = "D3D11";
                break;
            case nvrhi::GraphicsAPI::D3D12:
                name = "D3D12";
                break;
            case nvrhi::GraphicsAPI::VULKAN:
                name = "VULKAN";
                break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};

namespace bocchi
{
    Rhi* Rhi::Create(nvrhi::GraphicsAPI api)
    {
        switch (api)
        {
#if USE_DX11
            case nvrhi::GraphicsAPI::D3D11:
                return CreateD3D11();
#endif
#if USE_DX12
            case nvrhi::GraphicsAPI::D3D12:
                return CreateD3D12();
#endif
#if USE_VK
            case nvrhi::GraphicsAPI::VULKAN:
                return CreateVulkanRhi();
#endif
            default:
                LOG_ERROR("RHI::Create: Unsupported Graphics API (%d)", api);
                // LOG_ERROR("RHI::Create: Unsupported Graphics API");
                return nullptr;
        }
    }

    DefaultMessageCallback& DefaultMessageCallback::GetInstance()
    {
        static DefaultMessageCallback instance;
        return instance;
    }

    void DefaultMessageCallback::message(const nvrhi::MessageSeverity severity, const char* message_text)
    {
        switch (severity)
        {
            case nvrhi::MessageSeverity::Info:
                LOG_INFO(message_text)
                break;
            case nvrhi::MessageSeverity::Warning:
                LOG_WARN(message_text)
                break;
            case nvrhi::MessageSeverity::Error:
                LOG_ERROR(message_text)
                break;
            case nvrhi::MessageSeverity::Fatal:
                LOG_FATAL(message_text)
                break;
        }
    }
} // namespace bocchi