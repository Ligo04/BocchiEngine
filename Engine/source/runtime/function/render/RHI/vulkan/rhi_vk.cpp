#include "rhi_vk.h"
#include "runtime/function/render/window_system.h"
#include <limits>
#include <set>



namespace Bocchi
{
    VulkanRHI::~VulkanRHI()
    {
        // TODO
    }
    void VulkanRHI::initialize(RHIInitInfo initialize_info)
    {
        m_window = initialize_info.window_system->getWindow();


#ifdef NODEBUG
        m_enable_validation_layers = false;
#else
        m_enable_validation_layers = true;
#endif   // NODEBUG

        // create Instance
        createInstance();
        // setup debugger
        setupDebugMessenger();
        // create surface
        createSurface();
        // physical device
        pickPhysicsDevice();
        // logical device
        createLogicalDevice();
        // swap chain
        createSwapChain();
        // image view
        createImageViews();
    }
    void VulkanRHI::prepareContext()
    {
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRHI::debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      message_serverity,
        VkDebugUtilsMessageTypeFlagsEXT             message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_userdata)
    {
        LOG_ERROR("validation layer: " + p_callback_data->pMessage);
        return VK_FALSE;
    }

    void VulkanRHI::createInstance()
    {
        if (m_enable_validation_layers && !checkValidationLayerSupport())
        {
            LOG_ERROR("validation layers requested,but nor available!");
        }

        // applicationinfo t
        VkApplicationInfo vkinstance_app_info{};
        vkinstance_app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        vkinstance_app_info.pApplicationName   = "Bocchi_renderer";
        vkinstance_app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        vkinstance_app_info.pEngineName        = "Bocchi";
        vkinstance_app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
        vkinstance_app_info.apiVersion         = m_vulkan_version;
        // createinfo

        VkInstanceCreateInfo vkinstance_create_info{};
        vkinstance_create_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        vkinstance_create_info.pApplicationInfo = &vkinstance_app_info;

        VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
        if (m_enable_validation_layers)
        {
            vkinstance_create_info.enabledLayerCount =
                static_cast<uint32_t>(m_validation_layer.size());
            vkinstance_create_info.ppEnabledLayerNames = m_validation_layer.data();

            populateDebugMessengerCreateInfo(debug_create_info);
            vkinstance_create_info.pNext = &debug_create_info;
        }
        else
        {
            vkinstance_create_info.enabledLayerCount = 0;
            vkinstance_create_info.pNext             = nullptr;
        }


        auto extensions                                = getRequiredExtensions();
        vkinstance_create_info.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
        vkinstance_create_info.ppEnabledExtensionNames = extensions.data();

        if (vkCreateInstance(&vkinstance_create_info, nullptr, &m_instance) != VK_SUCCESS)
        {
            LOG_ERROR("vk create instance");
        }
    }

    void VulkanRHI::createSurface()
    {
        if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
        {
            LOG_ERROR("failed to create KHR surface!");
        }
    }

    void VulkanRHI::pickPhysicsDevice()
    {
        m_physical_device = VK_NULL_HANDLE;

        // enum physcis device
        uint32_t physical_device_count = 0;
        vkEnumeratePhysicalDevices(m_instance, &physical_device_count, nullptr);

        if (physical_device_count == 0)
        {
            LOG_ERROR("failed to find GPUS with Vulkan support!");
        }
        else
        {
            std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
            vkEnumeratePhysicalDevices(m_instance, &physical_device_count, physical_devices.data());

            // give device a score
            std::vector<std::pair<int, VkPhysicalDevice>> physical_device_candidates;

            for (const auto& device : physical_devices)
            {
                int score = rateDeviceSuitability(device);
                physical_device_candidates.push_back(std::make_pair(score, device));
            }

            std::sort(physical_device_candidates.begin(),
                      physical_device_candidates.end(),
                      [](const std::pair<int, VkPhysicalDevice>& p1,
                         const std::pair<int, VkPhysicalDevice>& p2)
                      { return p1.first > p2.first; });

            for (const auto& device : physical_device_candidates)
            {
                if (isDeviceSuitable(device.second))
                {
                    m_physical_device = device.second;
                    break;
                }
            }

            if (m_physical_device == VK_NULL_HANDLE)
            {
                LOG_ERROR("failed to find a suitable GPU!");
            }
        }
    }

    bool VulkanRHI::isDeviceSuitable(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices = findQueueFamilies(device);

        bool extension_supported = checkDeviceExtensionSupport(device);

        bool swapchain_adequate = false;
        if (extension_supported)
        {
            SwapChainSupportDetails swapchain_support = querySwapChainSupport(device);
            swapchain_adequate =
                !swapchain_support.formats.empty() && !swapchain_support.presentModes.empty();
        }

        return indices.isComplete() && extension_supported && swapchain_adequate;
    }

    int VulkanRHI::rateDeviceSuitability(VkPhysicalDevice device)
    {
        int score = 0;

        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(device, &device_properties);
        VkPhysicalDeviceFeatures device_features;
        vkGetPhysicalDeviceFeatures(device, &device_features);

        // discrete GPUs  have a sinificant performance advantage
        if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            score += 1000;
        }

        // Maximum possible size of textures affects graphics quality
        score += device_properties.limits.maxImageDimension2D;

        // Application can't function without geometry shaders
        if (!device_features.geometryShader)
        {
            return 0;
        }

        return score;
    }

    QueueFamilyIndices VulkanRHI::findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(
            device, &queue_family_count, queue_families.data());

        // including the type of operations that are supported and the number of queues that can be
        // created based on that family.

        uint32_t i = 0;
        for (const auto& queue_family : queue_families)
        {
            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphics_family = i;
            }

            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &present_support);

            if (present_support)
            {
                indices.present_family = i;
            }
            if (indices.isComplete())
            {
                break;
            }
            ++i;
        }

        return indices;
    }

    bool VulkanRHI::checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(
            device, nullptr, &extension_count, available_extensions.data());

        std::set<std::string> m_required_extensions(m_device_extensions.begin(),
                                                    m_device_extensions.end());

        for (const auto& extensions : available_extensions)
        {
            m_required_extensions.erase(extensions.extensionName);
        }

        return m_required_extensions.empty();
    }

    void VulkanRHI::createLogicalDevice()
    {
        m_queue_indics = findQueueFamilies(m_physical_device);

        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        std::set<uint32_t> unique_queue_families{m_queue_indics.graphics_family.value(),
                                                 m_queue_indics.present_family.value()};

        float queue_priority = 1.0f;
        for (uint32_t queue_family : unique_queue_families)
        {
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = queue_family;
            queue_create_info.queueCount       = 1;
            queue_create_info.pQueuePriorities = &queue_priority;
            queue_create_infos.push_back(queue_create_info);
        }


        VkPhysicalDeviceFeatures physical_device_features{};

        VkDeviceCreateInfo device_create_info{};
        device_create_info.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.pQueueCreateInfos    = queue_create_infos.data();
        device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
        device_create_info.pEnabledFeatures     = &physical_device_features;

        device_create_info.enabledExtensionCount =
            static_cast<uint32_t>(m_device_extensions.size());
        device_create_info.ppEnabledExtensionNames = m_device_extensions.data();
        if (m_enable_validation_layers)
        {
            device_create_info.enabledLayerCount = static_cast<uint32_t>(m_validation_layer.size());
            device_create_info.ppEnabledLayerNames = m_validation_layer.data();
        }
        else
        {
            device_create_info.enabledLayerCount = 0;
        }

        if (vkCreateDevice(m_physical_device, &device_create_info, nullptr, &m_device) !=
            VK_SUCCESS)
        {
            LOG_ERROR("failed to create device!");
        }

        vkGetDeviceQueue(m_device, m_queue_indics.graphics_family.value(), 0, &m_graphiy_queue);
        vkGetDeviceQueue(m_device, m_queue_indics.present_family.value(), 0, &m_graphiy_queue);
    }

    void VulkanRHI::createSwapChain()
    {
        SwapChainSupportDetails swap_chain_support = querySwapChainSupport(m_physical_device);

        VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swap_chain_support.formats);
        VkPresentModeKHR   present_mode   = chooseSwapPresentMode(swap_chain_support.presentModes);
        VkExtent2D         extent         = chooseSwapExtent(swap_chain_support.capabilities);

        uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;

        if (swap_chain_support.capabilities.maxImageCount > 0 &&
            image_count > swap_chain_support.capabilities.maxImageCount)
        {
            image_count = swap_chain_support.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR swap_chain_create_info{};
        swap_chain_create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swap_chain_create_info.surface          = m_surface;
        swap_chain_create_info.imageFormat      = surface_format.format;
        swap_chain_create_info.minImageCount    = image_count;
        swap_chain_create_info.imageColorSpace  = surface_format.colorSpace;
        swap_chain_create_info.imageExtent      = extent;
        swap_chain_create_info.imageArrayLayers = 1;
        swap_chain_create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queue_family_indices[] = {m_queue_indics.graphics_family.value(),
                                           m_queue_indics.present_family.value()};

        // use concurrent
        if (m_queue_indics.graphics_family != m_queue_indics.present_family)
        {
            swap_chain_create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            swap_chain_create_info.queueFamilyIndexCount = 2;
            swap_chain_create_info.pQueueFamilyIndices   = queue_family_indices;
        }
        else
        {
            swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        swap_chain_create_info.preTransform   = swap_chain_support.capabilities.currentTransform;
        swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swap_chain_create_info.presentMode    = present_mode;
        swap_chain_create_info.clipped        = VK_TRUE;
        swap_chain_create_info.oldSwapchain   = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_device, &swap_chain_create_info, nullptr, &m_swap_chain) !=
            VK_SUCCESS)
        {
            LOG_ERROR("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, nullptr);
        m_swap_chain_images.resize(image_count);
        vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, m_swap_chain_images.data());

        m_swap_chain_format = surface_format.format;
        m_swap_chain_extent = extent;
    }

    SwapChainSupportDetails VulkanRHI::querySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;

        // query the basic surface capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

        // query the basic supports surface formats
        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, nullptr);
        if (format_count != 0)
        {
            details.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(
                device, m_surface, &format_count, details.formats.data());
        }

        // query the basic supports surface present_mode
        uint32_t presentMode_Count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentMode_Count, nullptr);
        if (presentMode_Count != 0)
        {
            details.presentModes.resize(presentMode_Count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                device, m_surface, &presentMode_Count, details.presentModes.data());
        }


        return details;
    }

    VkSurfaceFormatKHR VulkanRHI::chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& available_formats)
    {
        for (const auto& available_format : available_formats)
        {
            if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
                available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return available_format;
            }
        }

        return available_formats[0];
    }

    VkPresentModeKHR VulkanRHI::chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR>& availabnle_present_modes)
    {
        for (const auto& available_present_mode : availabnle_present_modes)
        {
            // query the MAILBOX is available mode
            if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return available_present_mode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanRHI::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)())
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(m_window, &width, &height);

            VkExtent2D actual_extent = {static_cast<uint32_t>(width),
                                        static_cast<uint32_t>(height)};

            actual_extent.width = std::clamp(actual_extent.width,
                                             capabilities.minImageExtent.width,
                                             capabilities.maxImageExtent.width);


            actual_extent.height = std::clamp(actual_extent.height,
                                              capabilities.minImageExtent.height,
                                              capabilities.maxImageExtent.height);

            return actual_extent;
        }
    }

    void VulkanRHI::createImageViews()
    {
        m_swap_chain_image_views.resize(m_swap_chain_images.size());

        for (size_t i = 0; i < m_swap_chain_images.size(); ++i)
        {
            VkImageViewCreateInfo image_view_create_info{};
            image_view_create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            image_view_create_info.image    = m_swap_chain_images[i];
            image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            image_view_create_info.format   = m_swap_chain_format;

            image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            // no mipmapping
            image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            image_view_create_info.subresourceRange.baseMipLevel   = 0;
            image_view_create_info.subresourceRange.levelCount     = 1;
            image_view_create_info.subresourceRange.baseArrayLayer = 0;
            image_view_create_info.subresourceRange.layerCount     = 1;

            if (vkCreateImageView(
                    m_device, &image_view_create_info, nullptr, &m_swap_chain_image_views[i]) !=
                VK_SUCCESS)
            {
                LOG_ERROR("failed to creat image View!");
            }
        }
    }

    void VulkanRHI::setupDebugMessenger()
    {
        if (m_enable_validation_layers)
        {
            VkDebugUtilsMessengerCreateInfoEXT create_info{};
            populateDebugMessengerCreateInfo(create_info);

            if (createDebugUtilsMessengerEXT(
                    m_instance, &create_info, nullptr, &m_debug_messenger) != VK_SUCCESS)
            {
                LOG_ERROR("failed to set up debug messenger!");
            }
        }
    }

    bool VulkanRHI::checkValidationLayerSupport()
    {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        for (const char* layerName : m_validation_layer)
        {
            bool layerFound = false;
            for (const auto& layerProperties : available_layers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }

        return true;
    }

    std::vector<const char*> VulkanRHI::getRequiredExtensions()
    {
        uint32_t     glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (m_enable_validation_layers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    void VulkanRHI::populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT& create_info)
    {
        create_info                 = {};
        create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        create_info.pfnUserCallback = debugCallback;
    }

    VkResult VulkanRHI::createDebugUtilsMessengerEXT(
        VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
        const VkAllocationCallbacks* p_allocator, VkDebugUtilsMessengerEXT* p_debug_messenger)
    {
        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
        if (func != nullptr)
        {
            return func(instance, p_create_info, p_allocator, p_debug_messenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void VulkanRHI::destroyDebugUtilsMessengerEXT(VkInstance                   instance,
                                                  VkDebugUtilsMessengerEXT     debug_messenger,
                                                  const VkAllocationCallbacks* p_allocator)
    {
        auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (func != nullptr)
        {
            func(instance, debug_messenger, p_allocator);
        }
    }


    void VulkanRHI::clear()
    {
        for (auto& image_view : m_swap_chain_image_views)
        {
            vkDestroyImageView(m_device, image_view, nullptr);
        }
        if (m_enable_validation_layers)
        {
            destroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
        }
        vkDestroySwapchainKHR(m_device, m_swap_chain, nullptr);
        vkDestroyDevice(m_device, nullptr);
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        vkDestroyInstance(m_instance, nullptr);
    }
}   // namespace Bocchi