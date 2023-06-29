#pragma once
#include<nvrhi/nvrhi.h>
#include<nvrhi/vulkan.h>
#include<runtime/function/render/window_system.h>
namespace bocchi
{

    struct DefaultMessageCallback : public nvrhi::IMessageCallback
    {
        static DefaultMessageCallback& GetInstance();

        void message(nvrhi::MessageSeverity severity, const char* message_text) override;
    };

    struct RhiInitInfo
    {
        bool start_maximized   = false;
        bool start_full_screen = false;

        uint32_t      back_buffer_width             = 1280;
        uint32_t      back_buffer_height            = 720;
        uint32_t      refresh_rate                  = 0;
        uint32_t      swap_chain_buffer_count       = 3;
        nvrhi::Format swap_chain_format             = nvrhi::Format::SRGBA8_UNORM;
        uint32_t      swap_chain_sample_count       = 1;
        uint32_t      swap_chain_sample_quality     = 0;
        uint32_t      max_frame_in_flight           = 2;
        bool          enable_debug_runtiom          = false;
        bool          enable_nvrhi_validation_layer = false;
        bool          vsyn_enable                   = false;
        bool          enable_ray_tracing_extensions = false; // for vulkan
        bool          enable_compute_queue          = false;
        bool          enable_copy_queue             = false;

#if USE_DX11 || USE_DX12
        // Adapter to create the device on. Setting this to non-null overrides adapterNameSubstring.
        // If device creation fails on the specified adapter, it will *not* try any other adapters.
        IDXGIAdapter* adapter = nullptr;
#endif

        // For use in the case of multiple adapters; only effective if 'adapter' is null. If this is non-null, device
        // creation will try to match the given string against an adapter name.  If the specified string exists as a
        // sub-string of the adapter name, the device and window will be created on that adapter.  Case sensitive.
        std::wstring adapter_name_substring = L"";

        // set to true to enable DPI scale factors to be computed per monitor
        // this will keep the on-screen window size in pixels constant
        //
        // if set to false, the DPI scale factors will be constant but the system
        // may scale the contents of the window based on DPI
        //
        // note that the backbuffer size is never updated automatically; if the app
        // wishes to scale up rendering based on DPI, then it must set this to true
        // and respond to DPI scale factor changes by resizing the backbuffer explicitly
        bool enable_per_monitor_dpi = false;

#if USE_DX11 || USE_DX12
        DXGI_USAGE        swapChainUsage = DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT;
        D3D_FEATURE_LEVEL featureLevel   = D3D_FEATURE_LEVEL_11_1;
#endif

#if USE_VK
        std::vector<std::string>                   required_vulkan_instance_extensions;
        std::vector<std::string>                   required_vulkan_device_extensions;
        std::vector<std::string>                   required_vulkan_layers;
        std::vector<std::string>                   optional_vulkan_instance_extensions;
        std::vector<std::string>                   optional_vulkan_device_extensions;
        std::vector<std::string>                   optional_vulkan_layers;
        std::vector<size_t>                        ignored_vulkan_validation_message_locations;
        std::function<void(vk::DeviceCreateInfo&)> device_create_info_callback;
#endif
    };

    class WindowSystem;

    class Rhi
    {
    public:
        static Rhi* Create(nvrhi::GraphicsAPI api);

    	virtual bool Initialize(const RhiInitInfo& init_info,GLFWwindow* p_window) = 0;

    protected:
        // derived class need to realise
        virtual bool CreateDeviceAndSwapChain()  = 0;
        virtual void DestroyDeviceAndSwapChain() = 0;
        virtual void ResizeSwapChain()           = 0;
        virtual void BeginFrame()                = 0;
        virtual void Present()                   = 0;

    private:
        [[nodiscard]] virtual nvrhi::IDevice*    GetDevice() const       = 0;
        [[nodiscard]] virtual const char*        GetRendererString() const = 0;
        [[nodiscard]] virtual nvrhi::GraphicsAPI GetGraphicsApi() const  = 0;

        // back buffer
        [[nodiscard]] virtual nvrhi::ITexture* GetCurrentBackBuffer()        = 0;
        [[nodiscard]] virtual nvrhi::ITexture* GetBackBuffer(uint32_t index) = 0;
        [[nodiscard]] virtual uint32_t         GetCurrentBackBufferIndex()   = 0;
        [[nodiscard]] virtual uint32_t         GetBackBufferCount()          = 0;

    	virtual bool IsVulkanInstanceExtensionEnabled(const char* extension_name) const { return false; }
        virtual bool IsVulkanDeviceExtensionEnabled(const char* extension_name) const { return false; }
        virtual bool IsVulkanLayerEnabled(const char* layer_name) const { return false; }
        virtual void GetEnabledVulkanInstanceExtensions(std::vector<std::string>& extensions) const {}
        virtual void GetEnabledVulkanDeviceExtensions(std::vector<std::string>& extensions) const {}
        virtual void GetEnabledVulkanLayers(std::vector<std::string>& layers) const {}


    protected:
        RhiInitInfo m_rhi_creation_parameters_;
        GLFWwindow*           m_p_window_ = nullptr;

    private:
        static Rhi* CreateVulkanRhi();
    };
} // namespace bocchi
