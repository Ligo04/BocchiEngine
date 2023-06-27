#include "rhi.h"
#include <runtime/core/base/macro.h>

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
                LOG_ERROR("DeviceManager::Create: Unsupported Graphics API (%d)", api);
                return nullptr;
        }
	}

    DefaultMessageCallback& DefaultMessageCallback::GetInstance()
    {
        static DefaultMessageCallback Instance;
        return Instance;
    }

    void DefaultMessageCallback::message(nvrhi::MessageSeverity severity, const char* messageText)
    {
        switch (severity)
        {
            case nvrhi::MessageSeverity::Info:
                LOG_INFO(messageText);
                break;
            case nvrhi::MessageSeverity::Warning:
                LOG_WARN(messageText);
                break;
            case nvrhi::MessageSeverity::Error:
                LOG_ERROR(messageText);
                break;
            case nvrhi::MessageSeverity::Fatal:
                LOG_FATAL(messageText);
                break;
        }
    }
}