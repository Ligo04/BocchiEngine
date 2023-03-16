#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include "runtime/function/render/RHI/rhi.h"


#include <optional>


namespace Bocchi
{
    class VulkanRHI final : public RHI
    {

    public:
        VulkanRHI() = default;
        virtual ~VulkanRHI() override final;

        // initialize
        virtual void initialize(RHIInitInfo initialize_info) override final;
        virtual void prepareContext() override final;
        // destory
        virtual void clear() override;

    protected:
        void createInstance();

        // KHR_surface
        void createSurface();

        // physicsc device(GPU)
        void               pickPhysicsDevice();
        bool               isDeviceSuitable(VkPhysicalDevice device);
        int                rateDeviceSuitability(VkPhysicalDevice device);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        bool               checkDeviceExtensionSupport(VkPhysicalDevice device);
        // logic device
        void createLogicalDevice();
        // swap chain
        void                    createSwapChain();
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
        VkSurfaceFormatKHR      chooseSwapSurfaceFormat(
                 const std::vector<VkSurfaceFormatKHR>& available_formats);
        VkPresentModeKHR chooseSwapPresentMode(
            const std::vector<VkPresentModeKHR>& availabnle_present_modes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

        // image view
        void createImageViews();

    private:
        GLFWwindow* m_window{nullptr};

        VkInstance       m_instance{nullptr};
        VkPhysicalDevice m_physical_device{nullptr};
        VkDevice         m_device{nullptr};
        VkQueue          m_graphiy_queue{nullptr};
        VkSurfaceKHR     m_surface;
        VkSwapchainKHR   m_swap_chain;
        VkFormat         m_swap_chain_format;
        VkExtent2D       m_swap_chain_extent;

        std::vector<VkImage>     m_swap_chain_images{};
        std::vector<VkImageView> m_swap_chain_image_views{};

        const std::vector<const char*> m_device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        QueueFamilyIndices m_queue_indics;


        // debug
        bool                     checkValidationLayerSupport();
        std::vector<const char*> getRequiredExtensions();
        void                     setupDebugMessenger();
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT      message_serverity,
            VkDebugUtilsMessageTypeFlagsEXT             message_type,
            const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_userdata);

        VkResult createDebugUtilsMessengerEXT(
            VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
            const VkAllocationCallbacks* p_allocator, VkDebugUtilsMessengerEXT* p_debug_messenger);
        void destroyDebugUtilsMessengerEXT(VkInstance                   instance,
                                           VkDebugUtilsMessengerEXT     debug_messenger,
                                           const VkAllocationCallbacks* p_allocator);

        bool                           m_enable_validation_layers = {true};
        VkDebugUtilsMessengerEXT       m_debug_messenger;
        const std::vector<const char*> m_validation_layer = {"VK_LAYER_KHRONOS_validation"};
        uint32_t                       m_vulkan_version   = VK_API_VERSION_1_3;
    };
}   // namespace Bocchi