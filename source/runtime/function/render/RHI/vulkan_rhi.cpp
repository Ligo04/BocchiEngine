#include <nvrhi/validation.h>
#include <unordered_set>

#include "vulkan_rhi.h"
#include "runtime/core/base/macro.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace bocchi
{
    static std::vector<const char*> StringSetToVector(const std::unordered_set<std::string>& set)
    {
        std::vector<const char*> ret;
        ret.reserve(set.size());
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
        ret.reserve(set.size());
        for (const auto& s : set)
        {
            ret.push_back(s);
        }

        return ret;
    }

    void VulkanRhi::BeginFrame()
    {
        const vk::Result res = m_vulkan_device.acquireNextImageKHR(m_swap_chain,
                                                                    std::numeric_limits<uint64_t>::max(),
                                                                    m_present_semaphore,
                                                                    vk::Fence(),
                                                                    &m_swap_chain_index);

        assert(res == vk::Result::eSuccess);

        m_nvrhi_vk_device->queueWaitForSemaphore(nvrhi::CommandQueue::Graphics, m_present_semaphore, 0);
    }

    void VulkanRhi::Present()
    {
        m_nvrhi_vk_device->queueSignalSemaphore(nvrhi::CommandQueue::Graphics, m_present_semaphore, 0);

        // ummmm....
        m_barrier_command_list->open();
        m_barrier_command_list->close();
        
        vk::PresentInfoKHR info = vk::PresentInfoKHR()
                                      .setWaitSemaphoreCount(1)
                                      .setPWaitSemaphores(&m_present_semaphore)
                                      .setSwapchainCount(1)
                                      .setPSwapchains(&m_swap_chain)
                                      .setPImageIndices(&m_swap_chain_index);

        const vk::Result res = m_present_queue.presentKHR(&info);
        assert(res == vk::Result::eSuccess || res == vk::Result::eErrorOutOfDateKHR);

        if (m_rhi_creation_parameters.enable_debug_runtiom)
        {
            // according to vulkan-tutorial.com, "the validation layer implementation expects
            // the application to explicitly synchronize with the GPU"
            m_present_queue.waitIdle();
        }
        else
        {
#ifndef WIN32
            if (m_rhi_creation_parameters.vsyn_enable)
            {
                m_present_queue_.waitIdle();
            }
#endif
	        while (m_frame_in_flight.size()>m_rhi_creation_parameters.max_frame_in_flight)
	        {
                auto query = m_frame_in_flight.front();
                m_frame_in_flight.pop();

                m_nvrhi_vk_device->waitEventQuery(query);

                m_query_pool.push_back(query);
	        }

            nvrhi::EventQueryHandle query_handle;

            if (!m_query_pool.empty())
            {
                query_handle = m_query_pool.back();
                m_query_pool.pop_back();
            }
            else
            {
                query_handle = m_nvrhi_vk_device->createEventQuery();
            }

            m_nvrhi_vk_device->resetEventQuery(query_handle);
            m_nvrhi_vk_device->setEventQuery(query_handle, nvrhi::CommandQueue::Graphics);

            m_frame_in_flight.push(query_handle);

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
            m_enable_extension_set.instance.insert(glfw_ext[i]);
        }

        // add instance extensions requested by the user
        for (const std::string& name : m_rhi_creation_parameters.required_vulkan_instance_extensions)
        {
            m_enable_extension_set.instance.insert(name);
        }

        for (const std::string& name : m_rhi_creation_parameters.optional_vulkan_instance_extensions)
        {
            m_optional_extensions.instance.insert(name);
        }

        // add layers requested by the user
        for (const std::string& name : m_rhi_creation_parameters.required_vulkan_layers)
        {
            m_enable_extension_set.layers.insert(name);
        }

        for (const std::string& name : m_rhi_creation_parameters.optional_vulkan_layers)
        {
            m_optional_extensions.layers.insert(name);
        }

        std::unordered_set<std::string> required_extension = m_enable_extension_set.instance;

        for (const auto& instance_ext : vk::enumerateInstanceExtensionProperties())
        {
            const std::string name = instance_ext.extensionName;
            if (m_optional_extensions.instance.contains(name))
            {
                m_enable_extension_set.instance.insert(name);
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

        auto instance_ext_vec = StringSetToVector(m_enable_extension_set.instance);
        auto layer_vec        = StringSetToVector(m_enable_extension_set.layers);

        auto application_info = vk::ApplicationInfo();

        vk::Result res = vk::enumerateInstanceVersion(&application_info.apiVersion);

        if (res != vk::Result::eSuccess)
        {
            LOG_ERROR("Call to vkEnumerateInstanceVersion failed, error code = %s", nvrhi::vulkan::resultToString(res));
            return false;
        }

        // check if the vulkan api version is sufficient
        if ( constexpr uint32_t mini_vulkan_version = VK_MAKE_API_VERSION(0, 1, 3, 0) ; application_info.apiVersion < mini_vulkan_version)
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

        res = vk::createInstance(&instance_create_info, nullptr, &m_vulkan_instance);
        if (res != vk::Result::eSuccess)
        {
            LOG_ERROR("Failed to create a Vulkan instance, error code = %s", nvrhi::vulkan::resultToString(res));
            return false;
        }

        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_vulkan_instance);

        return true;
    }

    bool VulkanRhi::CreateWindowSurface()
    {
        if (glfwCreateWindowSurface(m_vulkan_instance, m_p_window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&m_windows_surface)) !=
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

        vk::Result res = m_vulkan_instance.createDebugReportCallbackEXT(&info, nullptr, &m_debug_report_callback_ext);
        assert(res == vk::Result::eSuccess);
    }

    bool VulkanRhi::PickPhysicalDevice()
    {
        vk::Format   requested_format = nvrhi::vulkan::convertFormat(m_rhi_creation_parameters.swap_chain_format);
        vk::Extent2D requested_extent(m_rhi_creation_parameters.back_buffer_width,
                                      m_rhi_creation_parameters.back_buffer_height);

        auto devices = m_vulkan_instance.enumeratePhysicalDevices();

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
            std::unordered_set<std::string> required_extensions = m_enable_extension_set.device;
            auto                            device_exts         = dev.enumerateDeviceExtensionProperties();
            for (const auto& ext : device_exts)
            {
                required_extensions.erase(ext.extensionName.data());
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
            auto surface_caps    = dev.getSurfaceCapabilitiesKHR(m_windows_surface);
            auto surface_fmts    = dev.getSurfaceFormatsKHR(m_windows_surface);
            auto surface_pmodels = dev.getSurfacePresentModesKHR(m_windows_surface);

            if (surface_caps.minImageCount > m_rhi_creation_parameters.swap_chain_buffer_count ||
                (surface_caps.maxImageCount < m_rhi_creation_parameters.swap_chain_buffer_count &&
                 surface_caps.maxImageCount > 0))
            {
                error_stream << std::endl << "  - cannot support the requested swap chain image count:";
                error_stream << " requested " << m_rhi_creation_parameters.swap_chain_buffer_count << ", available "
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
            m_vulkan_physical_device = discreate_gpus[0];
            return true;
        }

        if (!other_gpus.empty())
        {
            m_vulkan_physical_device = other_gpus[0];
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

            if (m_graphics_queue_family == -1)
            {
                if (queue_family.queueCount > 0 && (queue_family.queueFlags & vk::QueueFlagBits::eGraphics))
                {
                    m_graphics_queue_family = i;
                }
            }

            if (m_compute_queue_family == -1)
            {
                if (queue_family.queueCount > 0 && (queue_family.queueFlags & vk::QueueFlagBits::eCompute) &&
                    !(queue_family.queueFlags & vk::QueueFlagBits::eGraphics))
                {
                    m_compute_queue_family = i;
                }
            }

            if (m_transfer_queue_family == -1)
            {
                if (queue_family.queueCount > 0 && (queue_family.queueFlags & vk::QueueFlagBits::eTransfer) &&
                    !(queue_family.queueFlags & vk::QueueFlagBits::eCompute) &&
                    !(queue_family.queueFlags & vk::QueueFlagBits::eGraphics))
                {
                    m_transfer_queue_family = i;
                }
            }

            if (m_present_queue_family == -1)
            {
                if (queue_family.queueCount > 0 &&
                    glfwGetPhysicalDevicePresentationSupport(m_vulkan_instance, physical_device, i))
                {
                    m_present_queue_family = i;
                }
            }
        }

        return m_graphics_queue_family != -1 && m_present_queue_family != -1 &&
            (m_compute_queue_family != -1 || !m_rhi_creation_parameters.enable_compute_queue) &&
            (m_transfer_queue_family != -1 || !m_rhi_creation_parameters.enable_copy_queue);
    }

    bool VulkanRhi::CreateDevice()
    {
        // figure out which optional extensions are supported
        auto device_extensions = m_vulkan_physical_device.enumerateDeviceExtensionProperties();
        for (const auto& ext : device_extensions)
        {
            const std::string name = ext.extensionName;
            if (m_optional_extensions.device.contains(name) )
            {
                m_enable_extension_set.device.insert(name);
            }

            if (m_rhi_creation_parameters.enable_ray_tracing_extensions &&
                m_ray_tracing_extensions.contains(name) )
            {
                m_enable_extension_set.device.insert(name);
            }
        }

        const vk::PhysicalDeviceProperties physical_device_properties = m_vulkan_physical_device.getProperties();
        m_renderer_string = std::string(physical_device_properties.deviceName.data());

        bool accel_struct_supported     = false;
        bool ray_tracing_supported      = false;
        bool ray_query_supported        = false;
        bool meshlet_supported          = false;
        bool vrs_suppoted               = false;
        bool synchronization2_supported = false;
        bool maintenance4_supported     = false;

        for (const auto& ext : m_enable_extension_set.device)
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
        m_vulkan_physical_device.getFeatures2(&physical_device_features2);

        std::unordered_set<int> unique_queue_families = {m_graphics_queue_family, m_present_queue_family};

        if (m_rhi_creation_parameters.enable_compute_queue)
        {
            unique_queue_families.insert(m_compute_queue_family);
        }

        if (m_rhi_creation_parameters.enable_copy_queue)
        {
            unique_queue_families.insert(m_transfer_queue_family);
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

        auto layer_vec = StringSetToVector(m_enable_extension_set.layers);
        auto ext_vec   = StringSetToVector(m_enable_extension_set.device);

        auto device_desc = vk::DeviceCreateInfo()
                               .setPQueueCreateInfos(queue_desc.data())
                               .setQueueCreateInfoCount(static_cast<uint32_t>(queue_desc.size()))
                               .setPEnabledFeatures(&device_features)
                               .setEnabledExtensionCount(static_cast<uint32_t>(ext_vec.size()))
                               .setPpEnabledExtensionNames(ext_vec.data())
                               .setEnabledLayerCount(static_cast<uint32_t>(layer_vec.size()))
                               .setPpEnabledLayerNames(layer_vec.data())
                               .setPNext(&vulkan12_features);

        if (m_rhi_creation_parameters.device_create_info_callback)
            m_rhi_creation_parameters.device_create_info_callback(device_desc);

        const vk::Result res = m_vulkan_physical_device.createDevice(&device_desc, nullptr, &m_vulkan_device);

        if (res != vk::Result::eSuccess)
        {
            LOG_ERROR("Failed to create a Vulkan physical device, error code = %s", nvrhi::vulkan::resultToString(res));
            return false;
        }

        m_vulkan_device.getQueue(m_graphics_queue_family, 0, &m_graphics_queue);
        if (m_rhi_creation_parameters.enable_compute_queue)
        {
            m_vulkan_device.getQueue(m_compute_queue_family, 0, &m_compute_queue);
        }
        if (m_rhi_creation_parameters.enable_copy_queue)
        {
            m_vulkan_device.getQueue(m_transfer_queue_family, 0, &m_transfer_queue);
        }
        m_vulkan_device.getQueue(m_present_queue_family, 0, &m_present_queue);

        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_vulkan_device);

        // remember the bufferDeviceAddress feature enablement
        m_buffer_device_handle_address_supported = vulkan12_features.bufferDeviceAddress;

        return true;
    }

    bool VulkanRhi::CreateSwapChain()
    {
        DestroySwapChain();

        m_swap_chain_format = {nvrhi::vulkan::convertFormat(m_rhi_creation_parameters.swap_chain_format),
                                vk::ColorSpaceKHR::eSrgbNonlinear};

        vk::Extent2D extent =
            vk::Extent2D(m_rhi_creation_parameters.back_buffer_width, m_rhi_creation_parameters.back_buffer_height);

        std::unordered_set<uint32_t> unique_queues = {static_cast<UINT32>(m_graphics_queue_family),
                                                      static_cast<UINT32>(m_present_queue_family)};

        std::vector<uint32_t> queues = SetToVector(unique_queues);

        const bool enable_swap_chain_sharing = queues.size() > 1;

        auto desc = vk::SwapchainCreateInfoKHR()
                        .setSurface(m_windows_surface)
                        .setMinImageCount(m_rhi_creation_parameters.swap_chain_buffer_count)
                        .setImageFormat(m_swap_chain_format.format)
                        .setImageColorSpace(m_swap_chain_format.colorSpace)
                        .setImageExtent(extent)
                        .setImageArrayLayers(1)
                        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst |
                                       vk::ImageUsageFlagBits::eSampled)
                        .setImageSharingMode(enable_swap_chain_sharing ? vk::SharingMode::eConcurrent :
                                                                         vk::SharingMode::eExclusive)
                        .setQueueFamilyIndexCount(enable_swap_chain_sharing ? static_cast<uint32_t>(queues.size()) : 0)
                        .setPQueueFamilyIndices(enable_swap_chain_sharing ? queues.data() : nullptr)
                        .setPresentMode(m_rhi_creation_parameters.vsyn_enable ? vk::PresentModeKHR::eFifo :
                                                                                       vk::PresentModeKHR::eImmediate)
                        .setClipped(true)
                        .setOldSwapchain(nullptr);

        const vk::Result res = m_vulkan_device.createSwapchainKHR(&desc, nullptr, &m_swap_chain);
        if (res != vk::Result::eSuccess)
        {
            LOG_ERROR("Failed to create a Vulkan swap chain, error code = %s", nvrhi::vulkan::resultToString(res));
            return false;
        }

        // retrieve swap chain images

        for ( auto images = m_vulkan_device.getSwapchainImagesKHR(m_swap_chain) ; auto image : images)
        {
            SwapChainImage sci;
            sci.image = image;

            nvrhi::TextureDesc texture_desc;
            texture_desc.width            = m_rhi_creation_parameters.back_buffer_width;
            texture_desc.height           = m_rhi_creation_parameters.back_buffer_height;
            texture_desc.format           = m_rhi_creation_parameters.swap_chain_format;
            texture_desc.debugName        = "Swap chain image";
            texture_desc.initialState     = nvrhi::ResourceStates::Present;
            texture_desc.keepInitialState = true;
            texture_desc.isRenderTarget   = true;

            sci.rhi_handle = m_nvrhi_vk_device->createHandleForNativeTexture(
                nvrhi::ObjectTypes::VK_Image, nvrhi::Object(sci.image), texture_desc);

            m_swap_chain_images.push_back(sci);
        }

        m_swap_chain_index = 0;

        return true;
    }

    void VulkanRhi::DestroySwapChain()
    {
        if (m_vulkan_device)
        {
            m_vulkan_device.waitIdle();
        }

        if (m_swap_chain)
        {
            m_vulkan_device.destroySwapchainKHR(m_swap_chain);
            m_swap_chain = nullptr;
        }

        m_swap_chain_images.clear();
    }

    bool VulkanRhi::Initialize(const RhiInitInfo& init_info, GLFWwindow* p_window)
    {
        m_p_window                = p_window;
        m_rhi_creation_parameters = init_info;

        return CreateDeviceAndSwapChain();
    }

    bool VulkanRhi::CreateDeviceAndSwapChain()
    {
        if (m_rhi_creation_parameters.enable_debug_runtiom)
        {
            m_enable_extension_set.instance.insert("VK_EXT_debug_report");
            m_enable_extension_set.layers.insert("VK_LAYER_KHRONOS_validation");
        }

        const vk::DynamicLoader dynamic_loader;
        auto                    vk_get_instance_proc_addr =
            dynamic_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_get_instance_proc_addr);

#define CHECK(a) \
    if (!(a)) \
    { \
        return false; \
    }

        // create instance
        CHECK(CreateInstance())

        if (m_rhi_creation_parameters.enable_debug_runtiom)
        {
            InstallDebugCallBack();
        }

        // revort format
        if (m_rhi_creation_parameters.swap_chain_format == nvrhi::Format::SRGBA8_UNORM)
        {
            m_rhi_creation_parameters.swap_chain_format = nvrhi::Format::SBGRA8_UNORM;
        }
        else if (m_rhi_creation_parameters.swap_chain_format == nvrhi::Format::RGBA8_UNORM)
        {
            m_rhi_creation_parameters.swap_chain_format = nvrhi::Format::BGRA8_UNORM;
        }

        // add device extensions requested by the user
        for (const std::string& name : m_rhi_creation_parameters.required_vulkan_device_extensions)
        {
            m_enable_extension_set.device.insert(name);
        }
        for (const std::string& name : m_rhi_creation_parameters.optional_vulkan_device_extensions)
        {
            m_enable_extension_set.device.insert(name);
        }

        // create surface
        CHECK(CreateWindowSurface())
        // physical device
        CHECK(PickPhysicalDevice())
        // queue families
        CHECK(FindQueueFamilies(m_vulkan_physical_device))
        // logical device
        CHECK(CreateDevice())

        auto vec_instance_ext = StringSetToVector(m_enable_extension_set.instance);
        auto vec_layer_ext    = StringSetToVector(m_enable_extension_set.layers);
        auto vec_device_ext   = StringSetToVector(m_enable_extension_set.device);

        nvrhi::vulkan::DeviceDesc device_desc;
        device_desc.errorCB            = &DefaultMessageCallback::GetInstance();
        device_desc.instance           = m_vulkan_instance;
        device_desc.physicalDevice     = m_vulkan_physical_device;
        device_desc.device             = m_vulkan_device;
        device_desc.graphicsQueue      = m_graphics_queue;
        device_desc.graphicsQueueIndex = m_graphics_queue_family;
        if (m_rhi_creation_parameters.enable_compute_queue)
        {
            device_desc.computeQueue      = m_compute_queue;
            device_desc.computeQueueIndex = m_compute_queue_family;
        }
        if (m_rhi_creation_parameters.enable_copy_queue)
        {
            device_desc.transferQueue      = m_transfer_queue;
            device_desc.transferQueueIndex = m_transfer_queue_family;
        }
        device_desc.instanceExtensions           = vec_instance_ext.data();
        device_desc.numInstanceExtensions        = vec_instance_ext.size();
        device_desc.deviceExtensions             = vec_device_ext.data();
        device_desc.numDeviceExtensions          = vec_device_ext.size();
        device_desc.bufferDeviceAddressSupported = m_buffer_device_handle_address_supported;

        m_nvrhi_vk_device = nvrhi::vulkan::createDevice(device_desc);

        if (m_rhi_creation_parameters.enable_nvrhi_validation_layer)
        {
            m_validation_layer = nvrhi::validation::createValidationLayer(m_nvrhi_vk_device);
        }

        CHECK(CreateSwapChain());

        m_barrier_command_list = m_nvrhi_vk_device->createCommandList();

        m_present_semaphore = m_vulkan_device.createSemaphore(vk::SemaphoreCreateInfo());

#undef CHECK

        return true;
    }

    void VulkanRhi::DestroyDeviceAndSwapChain()
    {
        DestroySwapChain();
        m_vulkan_device.destroySemaphore(m_present_semaphore);
        m_present_semaphore = vk::Semaphore();

        m_nvrhi_vk_device  = nullptr;
        m_validation_layer = nullptr;
        m_renderer_string.clear();

        if (m_debug_report_callback_ext)
        {
            m_vulkan_instance.destroyDebugReportCallbackEXT(m_debug_report_callback_ext);
        }

        if (m_vulkan_device)
        {
            m_vulkan_device.destroy();
            m_vulkan_device = nullptr;
        }

        if (m_windows_surface)
        {
            assert(m_vulkan_instance);
            m_vulkan_instance.destroySurfaceKHR(m_windows_surface);
            m_windows_surface = nullptr;
        }

        if (m_vulkan_instance)
        {
            m_vulkan_instance.destroy();
            m_vulkan_device = nullptr;
        }
    }

    Rhi* Rhi::CreateVulkanRhi() { return new  VulkanRhi(); }

} // namespace bocchi
