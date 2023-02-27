#include "rhi_vk.h"
#include<map>


namespace Bocchi
{
    VulkanRHI::~VulkanRHI()
    {
        //TODO
    }
    void VulkanRHI::initialize(RHIInitInfo initialize_info)
    {
        //create Instance
        createInstane();
        //setup debugger
        setupDebugMessenger();
        //physical device
        pickPhysicsDevice();
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

    void VulkanRHI::createInstane()
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
        vkinstance_app_info.apiVersion         = VK_API_VERSION_1_0;
        // createinfo

        VkInstanceCreateInfo vkinstance_create_info{};
        vkinstance_create_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        vkinstance_create_info.pApplicationInfo = &vkinstance_app_info;

         VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
        if (m_enable_validation_layers)
        {
            vkinstance_create_info.enabledLayerCount =
                static_cast<uint32_t>(m_layer_validation.size());
            vkinstance_create_info.ppEnabledLayerNames = m_layer_validation.data();

            populateDebugMessengerCreateInfo(debug_create_info);
            vkinstance_create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
        }
        else
        {
            vkinstance_create_info.enabledLayerCount = 0;
            vkinstance_create_info.pNext             = nullptr;
        }


        auto extensions = getRequiredExtensions();
        vkinstance_create_info.enabledExtensionCount = (uint32_t)extensions.size();
        vkinstance_create_info.ppEnabledExtensionNames   = extensions.data();

        if (vkCreateInstance(&vkinstance_create_info, nullptr, &m_instance) != VK_SUCCESS)
        {
            LOG_ERROR("vk create instance");
        }
    }

    void VulkanRHI::pickPhysicsDevice()
    {
        m_physical_device = VK_NULL_HANDLE;

        //enum physcis device
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

            //give device a score
            std::vector<std::pair<int, VkPhysicalDevice>> physical_device_candidates;

            for (const auto& device : physical_devices)
            {
                int score = rateDeviceSuitability(device);
                physical_device_candidates.push_back(std::make_pair(score,device));
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

        return indices.isComplete();
    }

    int VulkanRHI::rateDeviceSuitability(VkPhysicalDevice device)
    {
        int score = 0;

        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(device, &device_properties);
        VkPhysicalDeviceFeatures device_features;
        vkGetPhysicalDeviceFeatures(device, &device_features);

        //discrete GPUs  have a sinificant performance advantage
        if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            score += 1000;
        }

        // Maximum possible size of textures affects graphics quality
        score += device_properties.limits.maxImageDimension2D;

        //Application can't function without geometry shaders
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

        //including the type of operations that are supported and the number of queues that can be
        //created based on that family.

        uint32_t i = 0;
        for (const auto& queue_family : queue_families)
        {
            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphics_family = i;
            }
            ++i;
        }

        return indices;
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

        for (const char* layerName : m_layer_validation)
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
        create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        create_info.pfnUserCallback = debugCallback;
        create_info.pUserData       = nullptr;   // Optional
    }

    VkResult VulkanRHI::createDebugUtilsMessengerEXT(
        VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
        const VkAllocationCallbacks* p_allocator, VkDebugUtilsMessengerEXT* p_debug_messenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            return func(instance, p_create_info, p_allocator, p_debug_messenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void VulkanRHI::DestroyDebugUtilsMessengerEXT(VkInstance                   instance,
                                                  VkDebugUtilsMessengerEXT     debug_messenger,
                                                  const VkAllocationCallbacks* p_allocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            func(instance, debug_messenger, p_allocator);
        }
    }


    void VulkanRHI::clear()
    {
        if (m_enable_validation_layers)
        {
            DestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
        }
        vkDestroyInstance(m_instance, nullptr);
    }
}   // namespace Bocchi