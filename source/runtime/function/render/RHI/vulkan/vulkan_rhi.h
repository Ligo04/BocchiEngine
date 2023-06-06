#pragma once
#include <map>

#include "runtime/function/render/RHI/rhi.h"
#include "runtime/function/render/RHI/vulkan/vulkan_rhi_resource.h"

#include <optional>

namespace Bocchi
{
    class VulkanRHI final : public RHI
    {

    public:
        VulkanRHI() = default;
        virtual ~VulkanRHI() override;

        // initialize
        virtual void initialize(RHIInitInfo initialize_info) override;
        virtual void prepareContext() override;

        bool allocateCommandBuffers(const RHICommandBufferAllocateInfo* p_allocate_info,
                                    RHICommandBuffer*&                  p_command_buffers) override;

        bool allocateDescriptorSets(const RHIDescriptorSetAllocateInfo* p_allocate_info,
                                    RHIDescriptorSet*&                  p_descriptor_sets) override;

        void createSwapchain() override;

        void recreateSwapchain() override;
        void createSwapchainImageViews() override;
        void createFramebufferImageAndView() override;

        RHISampler* getOrCreateDefaultSampler(RHIDefaultSamplerType type) override;

        RHISampler* getOrCreateMipmapSampler(uint32_t width, uint32_t height) override;

        RHIShader* createShaderModule(const std::vector<unsigned char>& shader_code) override;

        void createBuffer(RHIDeviceSize          size,
                          RHIBufferUsageFlags    usage,
                          RHIMemoryPropertyFlags properties,
                          RHIBuffer*&            buffer,
                          RHIDeviceMemory*&      buffer_memory) override;

        void createBufferAndInitialize(RHIBufferUsageFlags    usage,
                                       RHIMemoryPropertyFlags properties,
                                       RHIBuffer*&            buffer,
                                       RHIDeviceMemory*&      buffer_memory,
                                       RHIDeviceSize          size,
                                       void*                  data,
                                       int                    datasize) override;

        bool createBufferVma(VmaAllocator                   allocator,
                             const RHIBufferCreateInfo*     p_buffer_create_info,
                             const VmaAllocationCreateInfo* p_allocation_create_info,
                             RHIBuffer*&                    p_buffer,
                             VmaAllocation*                 p_allocation,
                             VmaAllocationInfo*             p_allocation_info) override;

        bool createBufferWithAlignmentVma(VmaAllocator                   allocator,
                                          const RHIBufferCreateInfo*     p_buffer_create_info,
                                          const VmaAllocationCreateInfo* p_allocation_create_info,
                                          RHIDeviceSize                  min_alignment,
                                          RHIBuffer*&                    p_buffer,
                                          VmaAllocation*                 p_allocation,
                                          VmaAllocationInfo*             p_allocation_info) override;

        void copyBuffer(RHIBuffer*    src_buffer,
                        RHIBuffer*    dst_buffer,
                        RHIDeviceSize src_offset,
                        RHIDeviceSize dst_offset,
                        RHIDeviceSize size) override;

        void createImage(uint32_t               image_width,
                         uint32_t               image_height,
                         RHIFormat              format,
                         RHIImageTiling         image_tiling,
                         RHIImageUsageFlags     image_usage_flags,
                         RHIMemoryPropertyFlags memory_property_flags,
                         RHIImage*&             image,
                         RHIDeviceMemory*&      memory,
                         RHIImageCreateFlags    image_create_flags,
                         uint32_t               array_layers,
                         uint32_t               miplevels) override;

        void createImageView(RHIImage*           image,
                             RHIFormat           format,
                             RHIImageAspectFlags image_aspect_flags,
                             RHIImageViewType    view_type,
                             uint32_t            layout_count,
                             uint32_t            miplevels,
                             RHIImageView*&      image_view) override;

        void createGlobalImage(RHIImage*&     image,
                               RHIImageView*& image_view,
                               VmaAllocation& image_allocation,
                               uint32_t       texture_image_width,
                               uint32_t       texture_image_height,
                               void*          texture_image_pixels,
                               RHIFormat      texture_image_format,
                               uint32_t       miplevels) override;

        void createCubeMap(RHIImage*&           image,
                           RHIImageView*&       image_view,
                           VmaAllocation&       image_allocation,
                           uint32_t             texture_image_width,
                           uint32_t             texture_image_height,
                           std::array<void*, 6> texture_image_pixels,
                           RHIFormat            texture_image_format,
                           uint32_t             miplevels) override;

        bool createCommandPool(const RHICommandPoolCreateInfo* p_create_info, RHICommandPool*& p_command_pool) override;

        bool createDescriptorPool(const RHIDescriptorPoolCreateInfo* p_create_info,
                                  RHIDescriptorPool*&                p_descriptor_pool) override;

        bool createDescriptorSetLayout(const RHIDescriptorSetLayoutCreateInfo* p_create_info,
                                       RHIDescriptorSetLayout*&                p_set_layout) override;

        bool createFence(const RHIFenceCreateInfo* p_create_info, RHIFence*& p_fence) override;

        bool createFramebuffer(const RHIFramebufferCreateInfo* p_create_info, RHIFramebuffer*& p_framebuffer) override;

        bool createGraphicsPipelines(RHIPipelineCache*                    pipeline_cache,
                                     uint32_t                             create_info_count,
                                     const RHIGraphicsPipelineCreateInfo* p_create_info,
                                     RHIPipeline*&                        p_pipelines) override;

        bool createComputePipelines(RHIPipelineCache*                   pipeline_cache,
                                    uint32_t                            create_info_count,
                                    const RHIComputePipelineCreateInfo* p_create_info,
                                    RHIPipeline*&                       p_pipelines) override;

        bool createPipelineLayout(const RHIPipelineLayoutCreateInfo* p_create_info,
                                  RHIPipelineLayout*&                p_pipeline_layout) override;

        bool createRenderPass(const RHIRenderPassCreateInfo* p_create_info, RHIRenderPass*& p_render_pass) override;

        bool createSampler(const RHISamplerCreateInfo* p_create_info, RHISampler*& p_sampler) override;

        bool createSemaphore(const RHISemaphoreCreateInfo* p_create_info, RHISemaphore*& p_semaphore) override;

        // command and command write
        bool
        waitForFencesPFN(uint32_t fence_count, RHIFence* const* p_fence, RHIBool32 wait_all, uint64_t timeout) override;
        bool resetFencesPFN(uint32_t fence_count, RHIFence* const* p_fences) override;
        bool resetCommandPoolPFN(RHICommandPool* command_pool, RHICommandPoolResetFlags flags) override;
        bool beginCommandBufferPFN(RHICommandBuffer*                command_buffer,
                                   const RHICommandBufferBeginInfo* p_begin_info) override;
        bool endCommandBufferPFN(RHICommandBuffer* command_buffer) override;
        void cmdBeginRenderPassPFN(RHICommandBuffer*             command_buffer,
                                   const RHIRenderPassBeginInfo* p_render_pass_begin,
                                   RHISubpassContents            contents) override;
        void cmdNextSubpassPFN(RHICommandBuffer* command_buffer, RHISubpassContents contents) override;
        void cmdEndRenderPassPFN(RHICommandBuffer* command_buffer) override;
        void cmdBindPipelinePFN(RHICommandBuffer*    command_buffer,
                                RHIPipelineBindPoint pipeline_bind_point,
                                RHIPipeline*         pipeline) override;
        void cmdSetViewportPFN(RHICommandBuffer*  command_buffer,
                               uint32_t           first_viewport,
                               uint32_t           viewport_count,
                               const RHIViewport* p_viewports) override;
        void cmdSetScissorPFN(RHICommandBuffer* command_buffer,
                              uint32_t          first_scissor,
                              uint32_t          scissor_count,
                              const RHIRect2D*  p_scissors) override;
        void cmdBindVertexBuffersPFN(RHICommandBuffer*    command_buffer,
                                     uint32_t             first_binding,
                                     uint32_t             binding_count,
                                     RHIBuffer* const*    p_buffers,
                                     const RHIDeviceSize* p_offsets) override;
        void cmdBindIndexBufferPFN(RHICommandBuffer* command_buffer,
                                   RHIBuffer*        buffer,
                                   RHIDeviceSize     offset,
                                   RHIIndexType      index_type) override;
        void cmdBindDescriptorSetsPFN(RHICommandBuffer*              command_buffer,
                                      RHIPipelineBindPoint           pipeline_bind_point,
                                      RHIPipelineLayout*             layout,
                                      uint32_t                       first_set,
                                      uint32_t                       descriptor_set_count,
                                      const RHIDescriptorSet* const* p_descriptor_sets,
                                      uint32_t                       dynamic_offset_count,
                                      const uint32_t*                p_dynamic_offsets) override;
        void cmdDrawIndexedPFN(RHICommandBuffer* command_buffer,
                               uint32_t          index_count,
                               uint32_t          instance_count,
                               uint32_t          first_index,
                               int32_t           vertex_offset,
                               uint32_t          first_instance) override;
        void cmdClearAttachmentsPFN(RHICommandBuffer*         command_buffer,
                                    uint32_t                  attachment_count,
                                    const RHIClearAttachment* p_attachments,
                                    uint32_t                  rect_count,
                                    const RHIClearRect*       p_rects) override;

        bool beginCommandBuffer(RHICommandBuffer*                command_buffer,
                                const RHICommandBufferBeginInfo* p_begin_info) override;
        void cmdCopyImageToBuffer(RHICommandBuffer*         command_buffer,
                                  RHIImage*                 src_image,
                                  RHIImageLayout            src_image_layout,
                                  RHIBuffer*                dst_buffer,
                                  uint32_t                  region_count,
                                  const RHIBufferImageCopy* p_regions) override;
        void cmdCopyImageToImage(RHICommandBuffer*      command_buffer,
                                 RHIImage*              src_image,
                                 RHIImageAspectFlagBits src_flag,
                                 RHIImage*              dst_image,
                                 RHIImageAspectFlagBits dst_flag,
                                 uint32_t               width,
                                 uint32_t               height) override;
        void cmdCopyBuffer(RHICommandBuffer* command_buffer,
                           RHIBuffer*        src_buffer,
                           RHIBuffer*        dst_buffer,
                           uint32_t          region_count,
                           RHIBufferCopy*    p_regions) override;
        void cmdDraw(RHICommandBuffer* command_buffer,
                     uint32_t          vertex_count,
                     uint32_t          instance_count,
                     uint32_t          first_vertex,
                     uint32_t          first_instance) override;
        void cmdDispatch(RHICommandBuffer* command_buffer,
                         uint32_t          group_count_x,
                         uint32_t          group_count_y,
                         uint32_t          group_count_z) override;
        void cmdDispatchIndirect(RHICommandBuffer* command_buffer, RHIBuffer* buffer, RHIDeviceSize offset) override;
        void cmdPipelineBarrier(RHICommandBuffer*             command_buffer,
                                RHIPipelineStageFlags         src_stage_mask,
                                RHIPipelineStageFlags         dst_stage_mask,
                                RHIDependencyFlags            dependency_flags,
                                uint32_t                      memory_barrier_count,
                                const RHIMemoryBarrier*       p_memory_barriers,
                                uint32_t                      buffer_memory_barrier_count,
                                const RHIBufferMemoryBarrier* p_buffer_memory_barriers,
                                uint32_t                      image_memory_barrier_count,
                                const RHIImageMemoryBarrier*  p_image_memory_barriers) override;
        bool endCommandBuffer(RHICommandBuffer* command_buffer) override;
        void updateDescriptorSets(uint32_t                     descriptor_write_count,
                                  const RHIWriteDescriptorSet* p_descriptor_writes,
                                  uint32_t                     descriptor_copy_count,
                                  const RHICopyDescriptorSet*  p_descriptor_copies) override;
        bool
        queueSubmit(RHIQueue* queue, uint32_t submit_count, const RHISubmitInfo* p_submits, RHIFence* fence) override;
        bool queueWaitIdle(RHIQueue* queue) override;
        void resetCommandPool() override;
        void waitForFences() override;
        bool waitForFences(uint32_t fence_count, const RHIFence* const* p_fences, RHIBool32 wait_all, uint64_t timeout);

        // query
        void                     getPhysicalDeviceProperties(RHIPhysicalDeviceProperties* pProperties) override;
        RHICommandBuffer*        getCurrentCommandBuffer() const override;
        RHICommandBuffer* const* getCommandBufferList() const override;
        RHICommandPool*          getCommandPoor() const override;
        RHIDescriptorPool*       getDescriptorPoor() const override;
        RHIFence* const*         getFenceList() const override;
        QueueFamilyIndices       getQueueFamilyIndices() const override;
        RHIQueue*                getGraphicsQueue() const override;
        RHIQueue*                getComputeQueue() const override;
        RHISwapChainDesc         getSwapchainInfo() override;
        RHIDepthImageDesc        getDepthImageInfo() const override;
        uint8_t                  getMaxFramesInFlight() const override;
        uint8_t                  getCurrentFrameIndex() const override;
        void                     setCurrentFrameIndex(uint8_t index) override;

        // command write
        RHICommandBuffer* beginSingleTimeCommands() override;
        void              endSingleTimeCommands(RHICommandBuffer* command_buffer) override;
        bool              prepareBeforePass(std::function<void()> pass_update_after_recreate_swapchain) override;
        void              submitRendering(std::function<void()> pass_update_after_recreate_swapchain) override;
        void              pushEvent(RHICommandBuffer* commond_buffer, const char* name, const float* color) override;
        void              popEvent(RHICommandBuffer* commond_buffer) override;

        // destory
        virtual void clear() override;
        void         clearSwapchain() override;
        void         destroyDefaultSampler(RHIDefaultSamplerType type) override;
        void         destroyMipmappedSampler() override;
        void         destroyShaderModule(RHIShader* shader_module) override;
        void         destroySemaphore(RHISemaphore* semaphore) override;
        void         destroySampler(RHISampler* sampler) override;
        void         destroyInstance(RHIInstance* instance) override;
        void         destroyImageView(RHIImageView* image_view) override;
        void         destroyImage(RHIImage* image) override;
        void         destroyFramebuffer(RHIFramebuffer* framebuffer) override;
        void         destroyFence(RHIFence* fence) override;
        void         destroyDevice() override;
        void         destroyCommandPool(RHICommandPool* command_pool) override;
        void         destroyBuffer(RHIBuffer*& buffer) override;
        void         freeCommandBuffers(RHICommandPool*   command_pool,
                                        uint32_t          command_buffer_count,
                                        RHICommandBuffer* p_command_buffers) override;

        // memory
        void freeMemory(RHIDeviceMemory*& memory) override;
        bool mapMemory(RHIDeviceMemory*  memory,
                       RHIDeviceSize     offset,
                       RHIDeviceSize     size,
                       RHIMemoryMapFlags flags,
                       void**            pp_data) override;
        void unmapMemory(RHIDeviceMemory* memory) override;
        void invalidateMappedMemoryRanges(void*            p_next,
                                          RHIDeviceMemory* memory,
                                          RHIDeviceSize    offset,
                                          RHIDeviceSize    size) override;
        void flushMappedMemoryRanges(void*            p_next,
                                     RHIDeviceMemory* memory,
                                     RHIDeviceSize    offset,
                                     RHIDeviceSize    size) override;

    protected:
        void createInstance();
        // KHR_surface
        void createWindowSurface();
        // physicsc device(GPU)
        void               pickPhysicsDevice();
        bool               isDeviceSuitable(VkPhysicalDevice device);
        int                rateDeviceSuitability(VkPhysicalDevice device);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        bool               checkDeviceExtensionSupport(VkPhysicalDevice device);
        // logic device
        void createLogicalDevice();
        // swap chain
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
        // command
        void createCommandPool() override;
        void createCommandBuffers();
        // dseciptor pool
        void createDescriptorPool();
        // sync
        void createSyncPrimitive();
        // assert allocator
        void createAssetAllocator();

    public:
        static uint8_t const k_max_frames_in_flight {3};

        GLFWwindow*      m_window {nullptr};
        VkInstance       m_instance {nullptr};
        VkPhysicalDevice m_physical_device {nullptr};
        VkDevice         m_device {nullptr};

        VkQueue            m_present_queue {nullptr};
        RHIQueue*          m_graphics_queue {nullptr};
        RHIQueue*          m_compute_queue {nullptr};
        QueueFamilyIndices m_queue_indices;

        VkSurfaceKHR m_surface;
        // swapchain
        RHIFormat                  m_swap_chain_format {RHI_FORMAT_UNDEFINED};
        RHIExtent2D                m_swap_chain_extent;
        VkSwapchainKHR             m_swap_chain;
        std::vector<VkImage>       m_swap_chain_images {};
        std::vector<RHIImageView*> m_swap_chain_image_views {};
        RHIViewport                m_viewport;

        uint32_t m_current_swapchain_image_index;
        // sicssor
        RHIRect2D m_scissor {};
        // TODO：SET
        VkCommandBuffer m_vk_current_command_buffer;
        uint32_t        m_current_frame_index {0};

        // depth image
        RHIImage*      m_depth_image = new VulkanImage();
        VkDeviceMemory m_depth_image_memory;
        RHIFormat      m_depth_image_format {RHI_FORMAT_UNDEFINED};
        RHIImageView*  m_depth_image_view = new RHIImageView();

        // command
        RHICommandPool*   m_rhi_command_pool;
        VkCommandPool     m_command_pools[k_max_frames_in_flight];
        VkCommandBuffer   m_vk_command_buffers[k_max_frames_in_flight];
        RHICommandBuffer* m_command_buffers[k_max_frames_in_flight];
        RHICommandBuffer* m_current_command_buffer = new VulkanCommandBuffer();
        RHISemaphore*     m_image_available_for_texturescopy_semaphores[k_max_frames_in_flight];
        VkSemaphore       m_image_available_for_render_semaphores[k_max_frames_in_flight];
        VkSemaphore       m_image_finished_for_presentation_semaphores[k_max_frames_in_flight];
        RHIFence*         m_rhi_is_frame_in_flight_fences[k_max_frames_in_flight];
        VkFence           m_is_frame_in_flight_fences[k_max_frames_in_flight];

        // desciptpools
        VkDescriptorPool   m_vk_descriptor_pool;
        RHIDescriptorPool* m_descriptor_pool;
        // asset allocator use VMA library
        VmaAllocator m_assets_allocator;

        // function pointers
        PFN_vkCmdBeginDebugUtilsLabelEXT _vkCmdBeginDebugUtilsLabelEXT;
        PFN_vkCmdEndDebugUtilsLabelEXT   _vkCmdEndDebugUtilsLabelEXT;
        PFN_vkWaitForFences              _vkWaitForFences;
        PFN_vkResetFences                _vkResetFences;
        PFN_vkResetCommandPool           _vkResetCommandPool;
        PFN_vkBeginCommandBuffer         _vkBeginCommandBuffer;
        PFN_vkEndCommandBuffer           _vkEndCommandBuffer;
        PFN_vkCmdBeginRenderPass         _vkCmdBeginRenderPass;
        PFN_vkCmdNextSubpass             _vkCmdNextSubpass;
        PFN_vkCmdEndRenderPass           _vkCmdEndRenderPass;
        PFN_vkCmdBindPipeline            _vkCmdBindPipeline;
        PFN_vkCmdSetViewport             _vkCmdSetViewport;
        PFN_vkCmdSetScissor              _vkCmdSetScissor;
        PFN_vkCmdBindVertexBuffers       _vkCmdBindVertexBuffers;
        PFN_vkCmdBindIndexBuffer         _vkCmdBindIndexBuffer;
        PFN_vkCmdBindDescriptorSets      _vkCmdBindDescriptorSets;
        PFN_vkCmdDrawIndexed             _vkCmdDrawIndexed;
        PFN_vkCmdClearAttachments        _vkCmdClearAttachments;

    private:
        VkFormat findDepthFormat();
        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                                     VkImageTiling                tiling,
                                     VkFormatFeatureFlags         features);

    private:
        const std::vector<const char*> m_device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        // debug
        bool                     checkValidationLayerSupport();
        std::vector<const char*> getRequiredExtensions();
        void                     setupDebugMessenger();
        void                     populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_serverity,
                                                            VkDebugUtilsMessageTypeFlagsEXT        message_type,
                                                            const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
                                                            void*                                       p_userdata);

        VkResult createDebugUtilsMessengerExt(VkInstance                                instance,
                                              const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
                                              const VkAllocationCallbacks*              p_allocator,
                                              VkDebugUtilsMessengerEXT*                 p_debug_messenger);
        void     destroyDebugUtilsMessengerExt(VkInstance                   instance,
                                               VkDebugUtilsMessengerEXT     debug_messenger,
                                               const VkAllocationCallbacks* p_allocator);

        VkSurfaceFormatKHR
        chooseSwapchainSurfaceFormatFromDetails(const std::vector<VkSurfaceFormatKHR>& available_surface_formats);
        VkPresentModeKHR
                   chooseSwapchainPresentModeFromDetails(const std::vector<VkPresentModeKHR>& available_present_modes);
        VkExtent2D chooseSwapchainExtentFromDetails(const VkSurfaceCapabilitiesKHR& capabilities);

    private:
        bool m_enable_validation_layers = {true};
        bool m_enable_debug_utils_label {true};
        bool m_enable_point_light_shadow {true};

        // used in descriptor pool creation
        uint32_t m_max_vertex_blending_mesh_count {256};
        uint32_t m_max_material_count {256};

        VkDebugUtilsMessengerEXT       m_debug_messenger;
        const std::vector<const char*> m_validation_layer = {"VK_LAYER_KHRONOS_validation"};
        uint32_t                       m_vulkan_version   = VK_API_VERSION_1_3;

        // default sampler cache
        RHISampler*                     m_linear_sampler  = nullptr;
        RHISampler*                     m_nearest_sampler = nullptr;
        std::map<uint32_t, RHISampler*> m_mipmap_sampler_map;
    };
} // namespace Bocchi