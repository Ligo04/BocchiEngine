#pragma once
#include <queue>
#include <string>
#include <unordered_set>
#include "rhi.h"
#include "runtime/core/base/macro.h"

namespace bocchi
{
    class VulkanRhi final : public Rhi
    {
    public:
        [[nodiscard]] nvrhi::IDevice* GetDevice() const override
        {
            if (m_validation_layer)
                return m_validation_layer;

            return m_nvrhi_vk_device;
        }

        [[nodiscard]] nvrhi::GraphicsAPI GetGraphicsApi() const override { return nvrhi::GraphicsAPI::VULKAN; }

        bool Initialize(const RhiInitInfo& init_info, GLFWwindow* p_window) override;

    protected:
        bool CreateDeviceAndSwapChain() override;
        void DestroyDeviceAndSwapChain() override;

        void ResizeSwapChain() override
        {
            if (m_vulkan_device)
            {
                DestroySwapChain();
                CreateSwapChain();
            }
        }

        nvrhi::ITexture* GetCurrentBackBuffer() override
        {
            return m_swap_chain_images[m_swap_chain_index].rhi_handle;
        }

        nvrhi::ITexture* GetBackBuffer(uint32_t index) override
        {
            if (index < m_swap_chain_images.size())
            {
                return m_swap_chain_images[index].rhi_handle;
            }
            return nullptr;
        }

        uint32_t GetCurrentBackBufferIndex() override { return m_swap_chain_index; }

        uint32_t GetBackBufferCount() override { return static_cast<uint32_t>(m_swap_chain_images.size()); }

        void BeginFrame() override;
        void Present() override;

        const char* GetRendererString() const override { return m_renderer_string.c_str(); }

        bool IsVulkanInstanceExtensionEnabled(const char* extension_name) const override
        {
            return m_enable_extension_set.instance.find(extension_name) != m_enable_extension_set.instance.end();
        }

        bool IsVulkanDeviceExtensionEnabled(const char* extension_name) const override
        {
            return m_enable_extension_set.device.find(extension_name) != m_enable_extension_set.device.end();
        }

        bool IsVulkanLayerEnabled(const char* layer_name) const override
        {
            return m_enable_extension_set.layers.find(layer_name) != m_enable_extension_set.layers.end();
        }

        void GetEnabledVulkanInstanceExtensions(std::vector<std::string>& extensions) const override
        {
            for (const auto& ext : m_enable_extension_set.instance)
            {
                extensions.push_back(ext);
            }
        }

        void GetEnabledVulkanDeviceExtensions(std::vector<std::string>& extensions) const override
        {
            for (const auto& ext : m_enable_extension_set.device)
            {
                extensions.push_back(ext);
            }
        }

        void GetEnabledVulkanLayers(std::vector<std::string>& layers) const override
        {
            for (const auto& ext : m_enable_extension_set.layers)
            {
                layers.push_back(ext);
            }
        }

    private:
        bool CreateInstance();
        bool CreateWindowSurface();
        void InstallDebugCallBack();
        bool PickPhysicalDevice();
        bool FindQueueFamilies(vk::PhysicalDevice physical_device);
        bool CreateDevice();
        bool CreateSwapChain();
        void DestroySwapChain();

        struct VulkanExtensionSet
        {
            std::unordered_set<std::string> instance;
            std::unordered_set<std::string> layers;
             std::unordered_set<std::string> device;
        };

        // mini set of required extension
        VulkanExtensionSet m_enable_extension_set = {
            {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME},
            {},
            {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_MAINTENANCE1_EXTENSION_NAME},
        };

        // optional extensions
        VulkanExtensionSet m_optional_extensions = {
            // instance
            {VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME},
            // layers
            {},
            // device
            {VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
             VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
             VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
             VK_NV_MESH_SHADER_EXTENSION_NAME,
             VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
             VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
             VK_KHR_MAINTENANCE_4_EXTENSION_NAME},
        };

        // raytracing extension
        std::unordered_set<std::string> m_ray_tracing_extensions = {VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                                                                     VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                                                                     VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
                                                                     VK_KHR_RAY_QUERY_EXTENSION_NAME,
                                                                     VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME};

        std::string m_renderer_string;

        vk::Instance               m_vulkan_instance;
        vk::DebugReportCallbackEXT m_debug_report_callback_ext;

        vk::PhysicalDevice m_vulkan_physical_device;

        int m_graphics_queue_family = -1;
        int m_compute_queue_family  = -1;
        int m_transfer_queue_family = -1;
        int m_present_queue_family  = -1;

        vk::Device m_vulkan_device;
        vk::Queue  m_graphics_queue;
        vk::Queue  m_compute_queue;
        vk::Queue  m_transfer_queue;
        vk::Queue  m_present_queue;

        vk::SurfaceKHR m_windows_surface;

        vk::SurfaceFormatKHR m_swap_chain_format;
        vk::SwapchainKHR     m_swap_chain;

        struct SwapChainImage
        {
            vk::Image            image;
            nvrhi::TextureHandle rhi_handle;
        };

        std::vector<SwapChainImage> m_swap_chain_images;
        uint32_t                    m_swap_chain_index = static_cast<uint32_t>(-1);

        nvrhi::vulkan::DeviceHandle m_nvrhi_vk_device;
        nvrhi::DeviceHandle         m_validation_layer;

        nvrhi::CommandListHandle m_barrier_command_list;
        vk::Semaphore            m_present_semaphore;

        std::queue<nvrhi::EventQueryHandle> m_frame_in_flight;
        std::vector<nvrhi::EventQueryHandle> m_query_pool;

        bool m_buffer_device_handle_address_supported = false;

    private:
        static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugReportFlagsEXT       /*flags*/,
                                                                  VkDebugReportObjectTypeEXT  /*obj_type*/,
                                                                  uint64_t                    /*obj*/,
                                                                  size_t      location,
                                                                  int32_t     code,
                                                                  const char* layer_prefix,
                                                                  const char* msg,
                                                                  void* user_data)
        {
            if ( auto manager = static_cast<const VulkanRhi*>(user_data))
            {
                const auto& ignored = manager->m_rhi_creation_parameters.ignored_vulkan_validation_message_locations;
                if ( const auto found = std::ranges::find(ignored, location) ; found != ignored.end())
                    return VK_FALSE;
            }

            LOG_ERROR("[Vulkan: location=0x%zx code=%d, layerPrefix='%s'] %s", location, code, layer_prefix, msg)

            return VK_FALSE;
        }
    };
} // namespace bocchi
