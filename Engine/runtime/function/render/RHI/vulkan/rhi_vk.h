#pragma once

#include "runtime/function/render/RHI/rhi.h"
#include "runtime/core/mico.h"

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
        void createInstane();

        void pickPhysicsDevice();
        bool isDeviceSuitable(VkPhysicalDevice device);
        int  rateDeviceSuitability(VkPhysicalDevice device);
        
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    private:
        VkInstance       m_instance{nullptr};
        VkPhysicalDevice m_physical_device{nullptr};
    private:
        //debug
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
        void DestroyDebugUtilsMessengerEXT(VkInstance                   instance,
                                           VkDebugUtilsMessengerEXT     debug_messenger,
                                           const VkAllocationCallbacks* p_allocator);

     private:
        bool                           m_enable_validation_layers = {true};
        VkDebugUtilsMessengerEXT       m_debug_messenger          = nullptr;
        const std::vector<const char*> m_layer_validation         = {"VK_LAYER_KHRONOS_validation"};
    };
}   // namespace Bocchi