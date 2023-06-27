#include <queue>

#include "rhi.h"
#include "runtime/core/base/macro.h"
#include <nvrhi/validation.h>
#include <unordered_set>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace bocchi
{
    static std::vector<const char*> StringSetToVector(const std::unordered_set<std::string>& set)
    {
        std::vector<const char*> ret;
        for (const auto& s : set)
        {
            ret.push_back(s.c_str());
        }

        return ret;
    }

    template<typename T>
    static std::vector<T> SetToVector(const std::unordered_set<T>& set)
    {
        std::vector<T> ret;
        for (const auto& s : set)
        {
            ret.push_back(s);
        }

        return ret;
    }

    class VulkanRhi : public Rhi
    {
    public:
        [[nodiscard]] nvrhi::IDevice* GetDevice() const override
        {
            if (m_validation_layer_)
                return m_validation_layer_;

            return m_nvrhi_vk_device_;
        }

        [[nodiscard]] nvrhi::GraphicsAPI GetGraphicsApi() const override { return nvrhi::GraphicsAPI::VULKAN; }

    protected:
        bool CreateDeviceAndSwapChain() override;
        void DestroyDeviceAndSwapChain() override;

        void ResizeSwapChain() override
        {
            if (m_vulkan_device_)
            {
                DestroySwapChain();
                CreateSwapChain();
            }
        }

        nvrhi::ITexture* GetCurrentBackBuffer() override
        {
            return m_swap_chain_images_[m_swap_chain_index_].rhi_handle;
        }

        nvrhi::ITexture* GetBackBuffer(uint32_t index) override
        {
            if (index < m_swap_chain_images_.size())
            {
                return m_swap_chain_images_[index].rhi_handle;
            }
            return nullptr;
        }

        uint32_t GetCurrentBackBufferIndex() override { return m_swap_chain_index_; }

        uint32_t GetBackBufferCount() override { return static_cast<uint32_t>(m_swap_chain_images_.size()); }

        void BeginFrame() override;
        void Present() override;

        const char* GetRendererString() const override { return m_renderer_string_.c_str(); }

        bool IsVulkanInstanceExtensionEnabled(const char* extension_name) const override
        {
	        return m_enable_extension_set_.instance.find(extension_name)!= m_enable_extension_set_.instance.end();
        }

        bool IsVulkanDeviceExtensionEnabled(const char* extension_name) const override
        {
	        return m_enable_extension_set_.device.find(extension_name)!= m_enable_extension_set_.device.end();
        }

        bool IsVulkanLayerEnabled(const char* layer_name) const override
        {
	        return m_enable_extension_set_.layers.find(layer_name)!= m_enable_extension_set_.layers.end();
        }

        void GetEnabledVulkanInstanceExtensions(std::vector<std::string>& extensions) const override
        {
            for (const auto& ext : m_enable_extension_set_.instance)
            {
	            extensions.push_back(ext);
            }
        }

        void GetEnabledVulkanDeviceExtensions(std::vector<std::string>& extensions) const override
        {
            for (const auto& ext : m_enable_extension_set_.device)
            {
                extensions.push_back(ext);
            }
        }

        void GetEnabledVulkanLayers(std::vector<std::string>& layers) const override
        {
            for (const auto& ext : m_enable_extension_set_.layers)
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
        VulkanExtensionSet m_enable_extension_set_ = {
            {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME},
            {},
            {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_MAINTENANCE1_EXTENSION_NAME},
        };

        // optional extensions
        VulkanExtensionSet m_optional_extensions_ = {
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
        std::unordered_set<std::string> m_ray_tracing_extensions_ = {VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                                                                     VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                                                                     VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
                                                                     VK_KHR_RAY_QUERY_EXTENSION_NAME,
                                                                     VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME};

        std::string m_renderer_string_;

        vk::Instance               m_vulkan_instance_;
        vk::DebugReportCallbackEXT m_debug_report_callback_ext_;

        vk::PhysicalDevice m_vulkan_physical_device_;

        int m_graphics_queue_family_ = -1;
        int m_compute_queue_family_  = -1;
        int m_transfer_queue_family_ = -1;
        int m_present_queue_family_  = -1;

        vk::Device m_vulkan_device_;
        vk::Queue  m_graphics_queue_;
        vk::Queue  m_compute_queue_;
        vk::Queue  m_transfer_queue_;
        vk::Queue  m_present_queue_;

        vk::SurfaceKHR m_windows_surface_;

        vk::SurfaceFormatKHR m_swap_chain_format_;
        vk::SwapchainKHR     m_swap_chain_;

        struct SwapChainImage
        {
            vk::Image            image;
            nvrhi::TextureHandle rhi_handle;
        };

        std::vector<SwapChainImage> m_swap_chain_images_;
        uint32_t                    m_swap_chain_index_ = static_cast<uint32_t>(-1);

        nvrhi::vulkan::DeviceHandle m_nvrhi_vk_device_;
        nvrhi::DeviceHandle         m_validation_layer_;

        nvrhi::CommandListHandle m_barrier_command_list_;
        vk::Semaphore            m_present_semaphore_;

        std::queue<nvrhi::EventQueryHandle>  m_frame_in_flight_;
        std::vector<nvrhi::EventQueryHandle> m_query_pool_;

        bool m_buffer_device_handle_address_supported_ = false;

    private:
        static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugReportFlagsEXT      flags,
                                                                  VkDebugReportObjectTypeEXT obj_type,
                                                                  uint64_t                   obj,
                                                                  size_t                     location,
                                                                  int32_t                    code,
                                                                  const char*                layer_prefix,
                                                                  const char*                msg,
                                                                  void*                      user_data)
        {
            if (const VulkanRhi* manager = (const VulkanRhi*)user_data)
            {
                const auto& ignored = manager->m_rhi_creation_parameters_.ignored_vulkan_validation_message_locations;
                const auto  found   = std::find(ignored.begin(), ignored.end(), location);
                if (found != ignored.end())
                    return VK_FALSE;
            }

            LOG_ERROR("[Vulkan: location=0x%zx code=%d, layerPrefix='%s'] %s", location, code, layer_prefix, msg);

            return VK_FALSE;
        }
    };

    void VulkanRhi::BeginFrame()
    {
        const vk::Result res = m_vulkan_device_.acquireNextImageKHR(m_swap_chain_,
                                                                    std::numeric_limits<uint64_t>::max(),
                                                                    m_present_semaphore_,
                                                                    vk::Fence(),
                                                                    &m_swap_chain_index_);

        assert(res == vk::Result::eSuccess);

        m_nvrhi_vk_device_->queueWaitForSemaphore(nvrhi::CommandQueue::Graphics, m_present_semaphore_, 0);
    }

    void VulkanRhi::Present()
    {
        m_nvrhi_vk_device_->queueSignalSemaphore(nvrhi::CommandQueue::Graphics, m_present_semaphore_, 0);

        // ummmm....
        m_barrier_command_list_->open();
        m_barrier_command_list_->close();

        vk::PresentInfoKHR info = vk::PresentInfoKHR()
                                      .setWaitSemaphoreCount(1)
                                      .setPWaitSemaphores(&m_present_semaphore_)
                                      .setSwapchainCount(1)
                                      .setPSwapchains(&m_swap_chain_)
                                      .setPImageIndices(&m_swap_chain_index_);

        const vk::Result res = m_present_queue_.presentKHR(&info);
        assert(res == vk::Result::eSuccess || res == vk::Result::eErrorOutOfDateKHR);

        if (m_rhi_creation_parameters_.enable_debug_runtiom)
        {
            // according to vulkan-tutorial.com, "the validation layer implementation expects
            // the application to explicitly synchronize with the GPU"
            m_present_queue_.waitIdle();
        }
        else
        {
#ifndef WIN32
            if (m_rhi_creation_parameters_.vsyn_enable)
            {
                m_present_queue_.waitIdle();
            }
#endif
	        while (m_frame_in_flight_.size()>m_rhi_creation_parameters_.max_frame_in_flight)
	        {
                auto query = m_frame_in_flight_.front();
                m_frame_in_flight_.pop();

                m_nvrhi_vk_device_->waitEventQuery(query);

                m_query_pool_.push_back(query);
	        }

            nvrhi::EventQueryHandle query_handle;

            if (!m_query_pool_.empty())
            {
                query_handle = m_query_pool_.back();
                m_query_pool_.pop_back();
            }
            else
            {
                query_handle = m_nvrhi_vk_device_->createEventQuery();
            }

            m_nvrhi_vk_device_->resetEventQuery(query_handle);
            m_nvrhi_vk_device_->setEventQuery(query_handle, nvrhi::CommandQueue::Graphics);

            m_frame_in_flight_.push(query_handle);

        }
    }

    bool VulkanRhi::CreateInstance()
    {
        if (!glfwVulkanSupported())
        {
            return false;
        }

        // add any extensions required by glfw
        uint32_t     glfw_ext_count;
        const char** glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
        assert(glfw_ext);

        for (uint32_t i = 0; i < glfw_ext_count; ++i)
        {
            m_enable_extension_set_.instance.insert(glfw_ext[i]);
        }

        // add instance extensions requested by the user
        for (const std::string& name : m_rhi_creation_parameters_.required_vulkan_instance_extensions)
        {
            m_enable_extension_set_.instance.insert(name);
        }

        for (const std::string& name : m_rhi_creation_parameters_.optional_vulkan_instance_extensions)
        {
            m_optional_extensions_.instance.insert(name);
        }

        // add layers requested by the user
        for (const std::string& name : m_rhi_creation_parameters_.required_vulkan_layers)
        {
            m_enable_extension_set_.layers.insert(name);
        }

        for (const std::string& name : m_rhi_creation_parameters_.optional_vulkan_layers)
        {
            m_optional_extensions_.layers.insert(name);
        }

        std::unordered_set<std::string> required_extension = m_enable_extension_set_.instance;

        for (const auto& instance_ext : vk::enumerateInstanceExtensionProperties())
        {
            const std::string name = instance_ext.extensionName;
            if (m_optional_extensions_.instance.find(name) != m_optional_extensions_.instance.end())
            {
                m_enable_extension_set_.instance.insert(name);
            }

            required_extension.erase(name);
        }

        if (!required_extension.empty())
        {
            std::stringstream ss;
            ss << "Cannot create a Vulkan instance because the following required extension(s) are not supported:";
            for (const auto& ext : required_extension)
            {
                ss << std::endl << "  - " << ext;
            }

            LOG_ERROR("%s", ss.str().c_str());
        }

        auto instance_ext_vec = StringSetToVector(m_enable_extension_set_.instance);
        auto layer_vec        = StringSetToVector(m_enable_extension_set_.layers);

        auto application_info = vk::ApplicationInfo();

        vk::Result res = vk::enumerateInstanceVersion(&application_info.apiVersion);

        if (res != vk::Result::eSuccess)
        {
            LOG_ERROR("Call to vkEnumerateInstanceVersion failed, error code = %s", nvrhi::vulkan::resultToString(res));
            return false;
        }

        const uint32_t mini_vulkan_version = VK_MAKE_API_VERSION(0, 1, 3, 0);

        // check if the vulkan api version is sufficient
        if (application_info.apiVersion < mini_vulkan_version)
        {
            LOG_ERROR(
                "The Vulkan API version supported on the system (%d.%d.%d) is too low, at least %d.%d.%d is required.",
                VK_API_VERSION_MAJOR(application_info.apiVersion),
                VK_API_VERSION_MINOR(application_info.apiVersion),
                VK_API_VERSION_PATCH(application_info.apiVersion),
                VK_API_VERSION_MAJOR(mini_vulkan_version),
                VK_API_VERSION_MINOR(mini_vulkan_version),
                VK_API_VERSION_PATCH(mini_vulkan_version));
            return false;
        }

        if (VK_API_VERSION_VARIANT(application_info.apiVersion) != 0)
        {
            LOG_ERROR("The Vulkan API supported on the system uses an unexpected variant: %d.",
                      VK_API_VERSION_VARIANT(application_info.apiVersion));
            return false;
        }

        // create vulkan instance
        vk::InstanceCreateInfo instance_create_info =
            vk::InstanceCreateInfo()
                .setEnabledLayerCount(static_cast<uint32_t>(layer_vec.size()))
                .setPpEnabledLayerNames(layer_vec.data())
                .setEnabledExtensionCount(static_cast<uint32_t>(instance_ext_vec.size()))
                .setPpEnabledExtensionNames(instance_ext_vec.data())
                .setPApplicationInfo(&application_info);

        res = vk::createInstance(&instance_create_info, nullptr, &m_vulkan_instance_);
        if (res != vk::Result::eSuccess)
        {
            LOG_ERROR("Failed to create a Vulkan instance, error code = %s", nvrhi::vulkan::resultToString(res));
            return false;
        }

        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_vulkan_instance_);

        return true;
    }

    bool VulkanRhi::CreateWindowSurface()
    {
        if (glfwCreateWindowSurface(m_vulkan_instance_, m_window_, nullptr, (VkSurfaceKHR*)&m_windows_surface_) !=
            VK_SUCCESS)
        {
            LOG_ERROR("failed to create KHR surface!");
        }
        return true;
    }

    void VulkanRhi::InstallDebugCallBack()
    {
        auto info = vk::DebugReportCallbackCreateInfoEXT()
                        .setFlags(vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning |
                                  vk::DebugReportFlagBitsEXT::ePerformanceWarning)
                        .setPfnCallback(VulkanDebugCallback)
                        .setPUserData(this);

        vk::Result res = m_vulkan_instance_.createDebugReportCallbackEXT(&info, nullptr, &m_debug_report_callback_ext_);
        assert(res == vk::Result::eSuccess);
    }

    bool VulkanRhi::PickPhysicalDevice()
    {
        vk::Format   requested_format = nvrhi::vulkan::convertFormat(m_rhi_creation_parameters_.swap_chain_format);
        vk::Extent2D requested_extent(m_rhi_creation_parameters_.back_buffer_width,
                                      m_rhi_creation_parameters_.back_buffer_height);

        auto devices = m_vulkan_instance_.enumeratePhysicalDevices();

        // Start building an error message in case we cannot find a device.
        std::stringstream error_stream;
        error_stream << "Cannot find a Vulkan device that supports all the required extensions and properties.";

        // build a list of gpu
        std::vector<vk::PhysicalDevice> discreate_gpus;
        std::vector<vk::PhysicalDevice> other_gpus;

        for (const auto& dev : devices)
        {
            auto prop = dev.getProperties();

            error_stream << std::endl << prop.deviceName.data() << ":";

            // check that all required device extension are present
            std::unordered_set<std::string> required_extensions = m_enable_extension_set_.device;
            auto                            device_exts         = dev.enumerateDeviceExtensionProperties();
            for (const auto& ext : device_exts)
            {
                required_extensions.erase(std::string(ext.extensionName.data()));
            }

            bool device_is_good = true;

            if (!required_extensions.empty())
            {
                // device is missing one or more required extensions
                for (const auto& ext : required_extensions)
                {
                    error_stream << std::endl << "  - missing " << ext;
                }
                device_is_good = false;
            }

            auto device_features = dev.getFeatures();
            if (!device_features.samplerAnisotropy)
            {
                // device is a toaster oven
                error_stream << std::endl << "  - does not support samplerAnisotropy";
                device_is_good = false;
            }
            if (!device_features.textureCompressionBC)
            {
                error_stream << std::endl << " - does not support textureCompressionBC";
                device_is_good = false;
            }

            // check that this device supports our intend swap chain creation parameters
            auto surface_caps    = dev.getSurfaceCapabilitiesKHR(m_windows_surface_);
            auto surface_fmts    = dev.getSurfaceFormatsKHR(m_windows_surface_);
            auto surface_pmodels = dev.getSurfacePresentModesKHR(m_windows_surface_);

            if (surface_caps.minImageCount > m_rhi_creation_parameters_.swap_chain_buffer_count ||
                (surface_caps.maxImageCount < m_rhi_creation_parameters_.swap_chain_buffer_count &&
                 surface_caps.maxImageCount > 0))
            {
                error_stream << std::endl << "  - cannot support the requested swap chain image count:";
                error_stream << " requested " << m_rhi_creation_parameters_.swap_chain_buffer_count << ", available "
                             << surface_caps.minImageCount << " - " << surface_caps.maxImageCount;
                device_is_good = false;
            }

            if (surface_caps.minImageExtent.width > requested_extent.width ||
                surface_caps.minImageExtent.height > requested_extent.height ||
                surface_caps.maxImageExtent.width < requested_extent.width ||
                surface_caps.maxImageExtent.height < requested_extent.height)
            {
                error_stream << std::endl << "  - cannot support the requested swap chain size:";
                error_stream << " requested " << requested_extent.width << "x" << requested_extent.height << ", ";
                error_stream << " available " << surface_caps.minImageExtent.width << "x"
                             << surface_caps.minImageExtent.height;
                error_stream << " - " << surface_caps.maxImageExtent.width << "x" << surface_caps.maxImageExtent.height;
                device_is_good = false;
            }

            bool surface_format_present = false;
            for (const vk::SurfaceFormatKHR& surface_format : surface_fmts)
            {
                if (surface_format.format == requested_format)
                {
                    surface_format_present = true;
                    break;
                }
            }

            if (!surface_format_present)
            {
                // can't create a swap chain using the format requested
                error_stream << std::endl << "  - does not support the requested swap chain format";
                device_is_good = false;
            }

            if (!FindQueueFamilies(dev))
            {
                // device doesn't have all the queue families we need
                error_stream << std::endl << "  - does not support the necessary queue types";
                device_is_good = false;
            }

            if (!device_is_good)
            {
                continue;
            }

            if (prop.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                // discreate_gpus
                discreate_gpus.push_back(dev);
            }
            else
            {
                other_gpus.push_back(dev);
            }
        }

        // pick the firest discrete GPU if it exists,otherwise the firest integrated GPU
        if (!discreate_gpus.empty())
        {
            m_vulkan_physical_device_ = discreate_gpus[0];
            return true;
        }

        if (!other_gpus.empty())
        {
            m_vulkan_physical_device_ = other_gpus[0];
            return true;
        }

        LOG_ERROR("%s", error_stream.str().c_str());

        return false;
    }

    bool VulkanRhi::FindQueueFamilies(vk::PhysicalDevice physical_device)
    {
        auto props = physical_device.getQueueFamilyProperties();

        for (int i = 0; i < static_cast<int>(props.size()); i++)
        {
            const auto& queue_family = props[i];

            if (m_graphics_queue_family_ == -1)
            {
                if (queue_family.queueCount > 0 && (queue_family.queueFlags & vk::QueueFlagBits::eGraphics))
                {
                    m_graphics_queue_family_ = i;
                }
            }

            if (m_compute_queue_family_ == -1)
            {
                if (queue_family.queueCount > 0 && (queue_family.queueFlags & vk::QueueFlagBits::eCompute) &&
                    !(queue_family.queueFlags & vk::QueueFlagBits::eGraphics))
                {
                    m_compute_queue_family_ = i;
                }
            }

            if (m_transfer_queue_family_ == -1)
            {
                if (queue_family.queueCount > 0 && (queue_family.queueFlags & vk::QueueFlagBits::eTransfer) &&
                    !(queue_family.queueFlags & vk::QueueFlagBits::eCompute) &&
                    !(queue_family.queueFlags & vk::QueueFlagBits::eGraphics))
                {
                    m_transfer_queue_family_ = i;
                }
            }

            if (m_present_queue_family_ == -1)
            {
                if (queue_family.queueCount > 0 &&
                    glfwGetPhysicalDevicePresentationSupport(m_vulkan_instance_, physical_device, i))
                {
                    m_present_queue_family_ = i;
                }
            }
        }

        if (m_graphics_queue_family_ == -1 || m_present_queue_family_ == -1 ||
            (m_compute_queue_family_ == -1 && m_rhi_creation_parameters_.enable_compute_queue) ||
            (m_transfer_queue_family_ == -1 && m_rhi_creation_parameters_.enable_copy_queue))
        {
            return false;
        }

        return true;
    }

    bool VulkanRhi::CreateDevice()
    {
        // figure out which optional extensions are supported
        auto device_extensions = m_vulkan_physical_device_.enumerateDeviceExtensionProperties();
        for (const auto& ext : device_extensions)
        {
            const std::string name = ext.extensionName;
            if (m_optional_extensions_.device.find(name) != m_optional_extensions_.device.end())
            {
                m_enable_extension_set_.device.insert(name);
            }

            if (m_rhi_creation_parameters_.enable_ray_tracing_extensions &&
                m_ray_tracing_extensions_.find(name) != m_ray_tracing_extensions_.end())
            {
                m_enable_extension_set_.device.insert(name);
            }
        }

        const vk::PhysicalDeviceProperties physical_device_properties = m_vulkan_physical_device_.getProperties();
        m_renderer_string_ = std::string(physical_device_properties.deviceName.data());

        bool accel_struct_supported     = false;
        bool ray_tracing_supported      = false;
        bool ray_query_supported        = false;
        bool meshlet_supported          = false;
        bool vrs_suppoted               = false;
        bool synchronization2_supported = false;
        bool maintenance4_supported     = false;

        for (const auto& ext : m_enable_extension_set_.device)
        {

            if (ext == VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
                accel_struct_supported = true;
            else if (ext == VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
                ray_tracing_supported = true;
            else if (ext == VK_KHR_RAY_QUERY_EXTENSION_NAME)
                ray_query_supported = true;
            else if (ext == VK_NV_MESH_SHADER_EXTENSION_NAME)
                meshlet_supported = true;
            else if (ext == VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME)
                vrs_suppoted = true;
            else if (ext == VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)
                synchronization2_supported = true;
            else if (ext == VK_KHR_MAINTENANCE_4_EXTENSION_NAME)
                maintenance4_supported = true;
        }

#define APPEND_EXTENSION(condition, desc) \
    if (condition) \
    { \
        (desc).pNext = p_next; \
        p_next       = &(desc); \
    }

        void* p_next = nullptr;

        vk::PhysicalDeviceFeatures2 physical_device_features2;
        // Determine support for Buffer Device Address, the vulkan 1.2 way
        auto buffer_device_address_features = vk::PhysicalDeviceBufferDeviceAddressFeatures();
        // Determine support for maintenance4
        auto maintenance4_features = vk::PhysicalDeviceMaintenance4Features();

        APPEND_EXTENSION(true, buffer_device_address_features);
        APPEND_EXTENSION(maintenance4_supported, maintenance4_features);

        physical_device_features2.pNext = p_next;
        m_vulkan_physical_device_.getFeatures2(&physical_device_features2);

        std::unordered_set<int> unique_queue_families = {m_graphics_queue_family_, m_present_queue_family_};

        if (m_rhi_creation_parameters_.enable_compute_queue)
        {
            unique_queue_families.insert(m_compute_queue_family_);
        }

        if (m_rhi_creation_parameters_.enable_copy_queue)
        {
            unique_queue_families.insert(m_transfer_queue_family_);
        }

        float                                  priority = 1.0f;
        std::vector<vk::DeviceQueueCreateInfo> queue_desc;
        queue_desc.reserve(unique_queue_families.size());
        for (int queue_family : unique_queue_families)
        {
            queue_desc.push_back(vk::DeviceQueueCreateInfo()
                                     .setQueueFamilyIndex(queue_family)
                                     .setQueueCount(1)
                                     .setPQueuePriorities(&priority));
        }

        auto accel_structu_featruess =
            vk::PhysicalDeviceAccelerationStructureFeaturesKHR().setAccelerationStructure(true);
        auto ray_tracing_pipeline_features = vk::PhysicalDeviceRayTracingPipelineFeaturesKHR()
                                                 .setRayTracingPipeline(true)
                                                 .setRayTraversalPrimitiveCulling(true);
        auto ray_query_features = vk::PhysicalDeviceRayQueryFeaturesKHR().setRayQuery(true);
        auto meshlet_features   = vk::PhysicalDeviceMeshShaderFeaturesNV().setTaskShader(true).setMeshShader(true);
        auto vrs_features       = vk::PhysicalDeviceFragmentShadingRateFeaturesKHR()
                                .setPipelineFragmentShadingRate(true)
                                .setPrimitiveFragmentShadingRate(true)
                                .setAttachmentFragmentShadingRate(true);
        auto vulkan3_feature = vk::PhysicalDeviceVulkan13Features()
                                   .setSynchronization2(synchronization2_supported)
                                   .setMaintenance4(maintenance4_features.maintenance4);

        p_next = nullptr;
        APPEND_EXTENSION(accel_struct_supported, accel_structu_featruess)
        APPEND_EXTENSION(ray_tracing_supported, ray_tracing_pipeline_features)
        APPEND_EXTENSION(ray_query_supported, ray_query_features)
        APPEND_EXTENSION(meshlet_supported, meshlet_features)
        APPEND_EXTENSION(vrs_suppoted, vrs_features)
        APPEND_EXTENSION(physical_device_properties.apiVersion >= VK_API_VERSION_1_3, vulkan3_feature)
        APPEND_EXTENSION(physical_device_properties.apiVersion < VK_API_VERSION_1_3 && maintenance4_supported,
                         maintenance4_features);
#undef APPEND_EXTENSION

        auto device_features = vk::PhysicalDeviceFeatures()
                                   .setShaderImageGatherExtended(true)
                                   .setSamplerAnisotropy(true)
                                   .setTessellationShader(true)
                                   .setTextureCompressionBC(true)
                                   .setGeometryShader(true)
                                   .setImageCubeArray(true)
                                   .setDualSrcBlend(true);

        auto vulkan12_features = vk::PhysicalDeviceVulkan12Features()
                                     .setDescriptorIndexing(true)
                                     .setRuntimeDescriptorArray(true)
                                     .setDescriptorBindingPartiallyBound(true)
                                     .setDescriptorBindingVariableDescriptorCount(true)
                                     .setTimelineSemaphore(true)
                                     .setShaderSampledImageArrayNonUniformIndexing(true)
                                     .setBufferDeviceAddress(buffer_device_address_features.bufferDeviceAddress)
                                     .setPNext(p_next);

        auto layer_vec = StringSetToVector(m_enable_extension_set_.layers);
        auto ext_vec   = StringSetToVector(m_enable_extension_set_.device);

        auto device_desc = vk::DeviceCreateInfo()
                               .setPQueueCreateInfos(queue_desc.data())
                               .setQueueCreateInfoCount(static_cast<uint32_t>(queue_desc.size()))
                               .setPEnabledFeatures(&device_features)
                               .setEnabledExtensionCount(static_cast<uint32_t>(ext_vec.size()))
                               .setPpEnabledExtensionNames(ext_vec.data())
                               .setEnabledLayerCount(static_cast<uint32_t>(layer_vec.size()))
                               .setPpEnabledLayerNames(layer_vec.data())
                               .setPNext(&vulkan12_features);

        if (m_rhi_creation_parameters_.device_create_info_callback)
            m_rhi_creation_parameters_.device_create_info_callback(device_desc);

        const vk::Result res = m_vulkan_physical_device_.createDevice(&device_desc, nullptr, &m_vulkan_device_);

        if (res != vk::Result::eSuccess)
        {
            LOG_ERROR("Failed to create a Vulkan physical device, error code = %s", nvrhi::vulkan::resultToString(res));
            return false;
        }

        m_vulkan_device_.getQueue(m_graphics_queue_family_, 0, &m_graphics_queue_);
        if (m_rhi_creation_parameters_.enable_compute_queue)
        {
            m_vulkan_device_.getQueue(m_compute_queue_family_, 0, &m_compute_queue_);
        }
        if (m_rhi_creation_parameters_.enable_copy_queue)
        {
            m_vulkan_device_.getQueue(m_transfer_queue_family_, 0, &m_transfer_queue_);
        }
        m_vulkan_device_.getQueue(m_present_queue_family_, 0, &m_present_queue_);

        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_vulkan_device_);

        // remember the bufferDeviceAddress feature enablement
        m_buffer_device_handle_address_supported_ = vulkan12_features.bufferDeviceAddress;

        return true;
    }

    bool VulkanRhi::CreateSwapChain()
    {
        DestroySwapChain();

        m_swap_chain_format_ = {nvrhi::vulkan::convertFormat(m_rhi_creation_parameters_.swap_chain_format),
                                vk::ColorSpaceKHR::eSrgbNonlinear};

        vk::Extent2D extent =
            vk::Extent2D(m_rhi_creation_parameters_.back_buffer_width, m_rhi_creation_parameters_.back_buffer_height);

        std::unordered_set<uint32_t> unique_queues = {static_cast<UINT32>(m_graphics_queue_family_),
                                                      static_cast<UINT32>(m_present_queue_family_)};

        std::vector<uint32_t> queues = SetToVector(unique_queues);

        const bool enable_swap_chain_sharing = queues.size() > 1;

        auto desc = vk::SwapchainCreateInfoKHR()
                        .setSurface(m_windows_surface_)
                        .setMinImageCount(m_rhi_creation_parameters_.swap_chain_buffer_count)
                        .setImageFormat(m_swap_chain_format_.format)
                        .setImageColorSpace(m_swap_chain_format_.colorSpace)
                        .setImageExtent(extent)
                        .setImageArrayLayers(1)
                        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst |
                                       vk::ImageUsageFlagBits::eSampled)
                        .setImageSharingMode(enable_swap_chain_sharing ? vk::SharingMode::eConcurrent :
                                                                         vk::SharingMode::eExclusive)
                        .setQueueFamilyIndexCount(enable_swap_chain_sharing ? static_cast<uint32_t>(queues.size()) : 0)
                        .setPQueueFamilyIndices(enable_swap_chain_sharing ? queues.data() : nullptr)
                        .setPresentMode(m_rhi_creation_parameters_.vsyn_enable ? vk::PresentModeKHR::eFifo :
                                                                                       vk::PresentModeKHR::eImmediate)
                        .setClipped(true)
                        .setOldSwapchain(nullptr);

        const vk::Result res = m_vulkan_device_.createSwapchainKHR(&desc, nullptr, &m_swap_chain_);
        if (res != vk::Result::eSuccess)
        {
            LOG_ERROR("Failed to create a Vulkan swap chain, error code = %s", nvrhi::vulkan::resultToString(res));
            return false;
        }

        // retrieve swap chain images
        auto images = m_vulkan_device_.getSwapchainImagesKHR(m_swap_chain_);

        for (auto image : images)
        {
            SwapChainImage sci;
            sci.image = image;

            nvrhi::TextureDesc texture_desc;
            texture_desc.width            = m_rhi_creation_parameters_.back_buffer_width;
            texture_desc.height           = m_rhi_creation_parameters_.back_buffer_height;
            texture_desc.format           = m_rhi_creation_parameters_.swap_chain_format;
            texture_desc.debugName        = "Swap chain image";
            texture_desc.initialState     = nvrhi::ResourceStates::Present;
            texture_desc.keepInitialState = true;
            texture_desc.isRenderTarget   = true;

            sci.rhi_handle = m_nvrhi_vk_device_->createHandleForNativeTexture(
                nvrhi::ObjectTypes::VK_Image, nvrhi::Object(sci.image), texture_desc);

            m_swap_chain_images_.push_back(sci);
        }

        m_swap_chain_index_ = 0;

        return true;
    }

    void VulkanRhi::DestroySwapChain()
    {
        if (m_vulkan_device_)
        {
            m_vulkan_device_.waitIdle();
        }

        if (m_swap_chain_)
        {
            m_vulkan_device_.destroySwapchainKHR(m_swap_chain_);
            m_swap_chain_ = nullptr;
        }

        m_swap_chain_images_.clear();
    }

    bool VulkanRhi::CreateDeviceAndSwapChain()
    {
        if (m_rhi_creation_parameters_.enable_debug_runtiom)
        {
            m_enable_extension_set_.instance.insert("VK_EXT_debug_report");
            m_enable_extension_set_.layers.insert("VK_LAYER_KHRONOS_validation");
        }

        const vk::DynamicLoader         dynamic_loader;
        const PFN_vkGetInstanceProcAddr vk_get_instance_proc_addr =
            dynamic_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_get_instance_proc_addr);

#define CHECK(a) \
    if (!(a)) \
    { \
        return false; \
    }

        // create instance
        CHECK(CreateInstance())

        if (m_rhi_creation_parameters_.enable_debug_runtiom)
        {
            InstallDebugCallBack();
        }

        // revort format
        if (m_rhi_creation_parameters_.swap_chain_format == nvrhi::Format::SRGBA8_UNORM)
        {
            m_rhi_creation_parameters_.swap_chain_format = nvrhi::Format::SBGRA8_UNORM;
        }
        else if (m_rhi_creation_parameters_.swap_chain_format == nvrhi::Format::RGBA8_UNORM)
        {
            m_rhi_creation_parameters_.swap_chain_format = nvrhi::Format::BGRA8_UNORM;
        }

        // add device extensions requested by the user
        for (const std::string& name : m_rhi_creation_parameters_.required_vulkan_device_extensions)
        {
            m_enable_extension_set_.device.insert(name);
        }
        for (const std::string& name : m_rhi_creation_parameters_.optional_vulkan_device_extensions)
        {
            m_enable_extension_set_.device.insert(name);
        }

        // create surface
        CHECK(CreateWindowSurface())
        // physical device
        CHECK(PickPhysicalDevice())
        // queue families
        CHECK(FindQueueFamilies(m_vulkan_physical_device_))
        // logical device
        CHECK(CreateDevice())

        auto vec_instance_ext = StringSetToVector(m_enable_extension_set_.instance);
        auto vec_layer_ext    = StringSetToVector(m_enable_extension_set_.layers);
        auto vec_device_ext   = StringSetToVector(m_enable_extension_set_.device);

        nvrhi::vulkan::DeviceDesc device_desc;
        device_desc.errorCB            = &DefaultMessageCallback::GetInstance();
        device_desc.instance           = m_vulkan_instance_;
        device_desc.physicalDevice     = m_vulkan_physical_device_;
        device_desc.device             = m_vulkan_device_;
        device_desc.graphicsQueue      = m_graphics_queue_;
        device_desc.graphicsQueueIndex = m_graphics_queue_family_;
        if (m_rhi_creation_parameters_.enable_compute_queue)
        {
            device_desc.computeQueue      = m_compute_queue_;
            device_desc.computeQueueIndex = m_compute_queue_family_;
        }
        if (m_rhi_creation_parameters_.enable_copy_queue)
        {
            device_desc.transferQueue      = m_transfer_queue_;
            device_desc.transferQueueIndex = m_transfer_queue_family_;
        }
        device_desc.instanceExtensions           = vec_instance_ext.data();
        device_desc.numInstanceExtensions        = vec_instance_ext.size();
        device_desc.deviceExtensions             = vec_device_ext.data();
        device_desc.numDeviceExtensions          = vec_device_ext.size();
        device_desc.bufferDeviceAddressSupported = m_buffer_device_handle_address_supported_;

        m_nvrhi_vk_device_ = nvrhi::vulkan::createDevice(device_desc);

        if (m_rhi_creation_parameters_.enable_nvrhi_validation_layer)
        {
            m_validation_layer_ = nvrhi::validation::createValidationLayer(m_nvrhi_vk_device_);
        }

        CHECK(CreateSwapChain());

        m_barrier_command_list_ = m_nvrhi_vk_device_->createCommandList();

        m_present_semaphore_ = m_vulkan_device_.createSemaphore(vk::SemaphoreCreateInfo());

#undef CHECK

        return true;
    }

    void VulkanRhi::DestroyDeviceAndSwapChain()
    {
        DestroySwapChain();
        m_vulkan_device_.destroySemaphore(m_present_semaphore_);
        m_present_semaphore_ = vk::Semaphore();

        m_nvrhi_vk_device_  = nullptr;
        m_validation_layer_ = nullptr;
        m_renderer_string_.clear();

        if (m_debug_report_callback_ext_)
        {
            m_vulkan_instance_.destroyDebugReportCallbackEXT(m_debug_report_callback_ext_);
        }

        if (m_vulkan_device_)
        {
            m_vulkan_device_.destroy();
            m_vulkan_device_ = nullptr;
        }

        if (m_windows_surface_)
        {
            assert(m_vulkan_instance_);
            m_vulkan_instance_.destroySurfaceKHR(m_windows_surface_);
            m_windows_surface_ = nullptr;
        }

        if (m_vulkan_instance_)
        {
            m_vulkan_instance_.destroy();
            m_vulkan_device_ = nullptr;
        }
    }

    Rhi* Rhi::CreateVulkanRhi() { return new VulkanRhi(); }

} // namespace bocchi
