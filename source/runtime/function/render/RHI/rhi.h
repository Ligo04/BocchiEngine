#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>

#include <functional>
#include <memory>
#include <vector>

#include "rhi_struct.h"
namespace Bocchi
{
    class WindowSystem;

    struct RHIInitInfo
    {
        std::shared_ptr<WindowSystem> window_system;
    };

    class RHI
    {
    public:
        virtual ~RHI() = 0;

        virtual void initialize(RHIInitInfo initialize_info) = 0;
        virtual void prepareContext()                        = 0;

        // allocate and create
        virtual bool        allocateCommandBuffers(const RHICommandBufferAllocateInfo* p_allocate_info,
                                                   RHICommandBuffer*&                  p_command_buffers)                           = 0;
        virtual bool        allocateDescriptorSets(const RHIDescriptorSetAllocateInfo* p_allocate_info,
                                                   RHIDescriptorSet*&                  p_descriptor_sets)                           = 0;
        virtual void        createSwapchain()                                                                      = 0;
        virtual void        recreateSwapchain()                                                                    = 0;
        virtual void        createSwapchainImageViews()                                                            = 0;
        virtual void        createFramebufferImageAndView()                                                        = 0;
        virtual RHISampler* getOrCreateDefaultSampler(RHIDefaultSamplerType type)                                  = 0;
        virtual RHISampler* getOrCreateMipmapSampler(uint32_t width, uint32_t height)                              = 0;
        virtual RHIShader*  createShaderModule(const std::vector<unsigned char>& shader_code)                      = 0;
        virtual void        createBuffer(RHIDeviceSize          size,
                                         RHIBufferUsageFlags    usage,
                                         RHIMemoryPropertyFlags properties,
                                         RHIBuffer*&            buffer,
                                         RHIDeviceMemory*&      buffer_memory)                                          = 0;
        virtual void        createBufferAndInitialize(RHIBufferUsageFlags    usage,
                                                      RHIMemoryPropertyFlags properties,
                                                      RHIBuffer*&            buffer,
                                                      RHIDeviceMemory*&      buffer_memory,
                                                      RHIDeviceSize          size,
                                                      void*                  data     = nullptr,
                                                      int                    datasize = 0)                                            = 0;
        virtual bool        createBufferVma(VmaAllocator                   allocator,
                                            const RHIBufferCreateInfo*     p_buffer_create_info,
                                            const VmaAllocationCreateInfo* p_allocation_create_info,
                                            RHIBuffer*&                    p_buffer,
                                            VmaAllocation*                 p_allocation,
                                            VmaAllocationInfo*             p_allocation_info)                                  = 0;
        virtual bool        createBufferWithAlignmentVma(VmaAllocator                   allocator,
                                                         const RHIBufferCreateInfo*     p_buffer_create_info,
                                                         const VmaAllocationCreateInfo* p_allocation_create_info,
                                                         RHIDeviceSize                  min_alignment,
                                                         RHIBuffer*&                    p_buffer,
                                                         VmaAllocation*                 p_allocation,
                                                         VmaAllocationInfo*             p_allocation_info)                     = 0;
        virtual void        copyBuffer(RHIBuffer*    src_buffer,
                                       RHIBuffer*    dst_buffer,
                                       RHIDeviceSize src_offset,
                                       RHIDeviceSize dst_offset,
                                       RHIDeviceSize size)                                                         = 0;
        virtual void        createImage(uint32_t               image_width,
                                        uint32_t               image_height,
                                        RHIFormat              format,
                                        RHIImageTiling         image_tiling,
                                        RHIImageUsageFlags     image_usage_flags,
                                        RHIMemoryPropertyFlags memory_property_flags,
                                        RHIImage*&             image,
                                        RHIDeviceMemory*&      memory,
                                        RHIImageCreateFlags    image_create_flags,
                                        uint32_t               array_layers,
                                        uint32_t               miplevels)                                                        = 0;
        virtual void        createImageView(RHIImage*           image,
                                            RHIFormat           format,
                                            RHIImageAspectFlags image_aspect_flags,
                                            RHIImageViewType    view_type,
                                            uint32_t            layout_count,
                                            uint32_t            miplevels,
                                            RHIImageView*&      image_view)                                             = 0;
        virtual void        createGlobalImage(RHIImage*&     image,
                                              RHIImageView*& image_view,
                                              VmaAllocation& image_allocation,
                                              uint32_t       texture_image_width,
                                              uint32_t       texture_image_height,
                                              void*          texture_image_pixels,
                                              RHIFormat      texture_image_format,
                                              uint32_t       miplevels = 0)                                              = 0;
        virtual void        createCubeMap(RHIImage*&           image,
                                          RHIImageView*&       image_view,
                                          VmaAllocation&       image_allocation,
                                          uint32_t             texture_image_width,
                                          uint32_t             texture_image_height,
                                          std::array<void*, 6> texture_image_pixels,
                                          RHIFormat            texture_image_format,
                                          uint32_t             miplevels)                                                      = 0;
        virtual void        createCommandPool()                                                                    = 0;
        virtual bool        createCommandPool(const RHICommandPoolCreateInfo* p_create_info,
                                              RHICommandPool*&                p_command_pool)                                     = 0;
        virtual bool        createDescriptorPool(const RHIDescriptorPoolCreateInfo* p_create_info,
                                                 RHIDescriptorPool*&                p_descriptor_pool)                            = 0;
        virtual bool        createDescriptorSetLayout(const RHIDescriptorSetLayoutCreateInfo* p_create_info,
                                                      RHIDescriptorSetLayout*&                p_set_layout)                       = 0;
        virtual bool        createFence(const RHIFenceCreateInfo* p_create_info, RHIFence*& p_fence)               = 0;
        virtual bool        createFramebuffer(const RHIFramebufferCreateInfo* p_create_info,
                                              RHIFramebuffer*&                p_framebuffer)                                      = 0;
        virtual bool        createGraphicsPipelines(RHIPipelineCache*                    pipeline_cache,
                                                    uint32_t                             create_info_count,
                                                    const RHIGraphicsPipelineCreateInfo* p_create_info,
                                                    RHIPipeline*&                        p_pipelines)                                     = 0;
        virtual bool        createComputePipelines(RHIPipelineCache*                   pipeline_cache,
                                                   uint32_t                            create_info_count,
                                                   const RHIComputePipelineCreateInfo* p_create_info,
                                                   RHIPipeline*&                       p_pipelines)                                      = 0;
        virtual bool        createPipelineLayout(const RHIPipelineLayoutCreateInfo* p_create_info,
                                                 RHIPipelineLayout*&                p_pipeline_layout)                            = 0;
        virtual bool createRenderPass(const RHIRenderPassCreateInfo* p_create_info, RHIRenderPass*& p_render_pass) = 0;
        virtual bool createSampler(const RHISamplerCreateInfo* p_create_info, RHISampler*& p_sampler)              = 0;
        virtual bool createSemaphore(const RHISemaphoreCreateInfo* p_create_info, RHISemaphore*& p_semaphore)      = 0;

        // command and command write
        virtual bool
        waitForFencesPFN(uint32_t fence_count, RHIFence* const* p_fence, RHIBool32 wait_all, uint64_t timeout) = 0;
        virtual bool resetFencesPFN(uint32_t fence_count, RHIFence* const* p_fences)                           = 0;
        virtual bool resetCommandPoolPFN(RHICommandPool* command_pool, RHICommandPoolResetFlags flags)         = 0;
        virtual bool beginCommandBufferPFN(RHICommandBuffer*                command_buffer,
                                           const RHICommandBufferBeginInfo* p_begin_info)                      = 0;
        virtual bool endCommandBufferPFN(RHICommandBuffer* command_buffer)                                     = 0;
        virtual void cmdBeginRenderPassPFN(RHICommandBuffer*             command_buffer,
                                           const RHIRenderPassBeginInfo* p_render_pass_begin,
                                           RHISubpassContents            contents)                                        = 0;
        virtual void cmdNextSubpassPFN(RHICommandBuffer* command_buffer, RHISubpassContents contents)          = 0;
        virtual void cmdEndRenderPassPFN(RHICommandBuffer* command_buffer)                                     = 0;
        virtual void cmdBindPipelinePFN(RHICommandBuffer*    command_buffer,
                                        RHIPipelineBindPoint pipeline_bind_point,
                                        RHIPipeline*         pipeline)                                                 = 0;
        virtual void cmdSetViewportPFN(RHICommandBuffer*  command_buffer,
                                       uint32_t           first_viewport,
                                       uint32_t           viewport_count,
                                       const RHIViewport* p_viewports)                                         = 0;
        virtual void cmdSetScissorPFN(RHICommandBuffer* command_buffer,
                                      uint32_t          first_scissor,
                                      uint32_t          scissor_count,
                                      const RHIRect2D*  p_scissors)                                             = 0;
        virtual void cmdBindVertexBuffersPFN(RHICommandBuffer*    command_buffer,
                                             uint32_t             first_binding,
                                             uint32_t             binding_count,
                                             RHIBuffer* const*    p_buffers,
                                             const RHIDeviceSize* p_offsets)                                   = 0;
        virtual void cmdBindIndexBufferPFN(RHICommandBuffer* command_buffer,
                                           RHIBuffer*        buffer,
                                           RHIDeviceSize     offset,
                                           RHIIndexType      index_type)                                            = 0;
        virtual void cmdBindDescriptorSetsPFN(RHICommandBuffer*              command_buffer,
                                              RHIPipelineBindPoint           pipeline_bind_point,
                                              RHIPipelineLayout*             layout,
                                              uint32_t                       first_set,
                                              uint32_t                       descriptor_set_count,
                                              const RHIDescriptorSet* const* p_descriptor_sets,
                                              uint32_t                       dynamic_offset_count,
                                              const uint32_t*                p_dynamic_offsets)                               = 0;
        virtual void cmdDrawIndexedPFN(RHICommandBuffer* command_buffer,
                                       uint32_t          index_count,
                                       uint32_t          instance_count,
                                       uint32_t          first_index,
                                       int32_t           vertex_offset,
                                       uint32_t          first_instance)                                                = 0;
        virtual void cmdClearAttachmentsPFN(RHICommandBuffer*         command_buffer,
                                            uint32_t                  attachment_count,
                                            const RHIClearAttachment* p_attachments,
                                            uint32_t                  rect_count,
                                            const RHIClearRect*       p_rects)                                       = 0;

        virtual bool beginCommandBuffer(RHICommandBuffer*                command_buffer,
                                        const RHICommandBufferBeginInfo* p_begin_info)                              = 0;
        virtual void cmdCopyImageToBuffer(RHICommandBuffer*         command_buffer,
                                          RHIImage*                 src_image,
                                          RHIImageLayout            src_image_layout,
                                          RHIBuffer*                dst_buffer,
                                          uint32_t                  region_count,
                                          const RHIBufferImageCopy* p_regions)                                      = 0;
        virtual void cmdCopyImageToImage(RHICommandBuffer*      command_buffer,
                                         RHIImage*              src_image,
                                         RHIImageAspectFlagBits src_flag,
                                         RHIImage*              dst_image,
                                         RHIImageAspectFlagBits dst_flag,
                                         uint32_t               width,
                                         uint32_t               height)                                                           = 0;
        virtual void cmdCopyBuffer(RHICommandBuffer* command_buffer,
                                   RHIBuffer*        src_buffer,
                                   RHIBuffer*        dst_buffer,
                                   uint32_t          region_count,
                                   RHIBufferCopy*    p_regions)                                                        = 0;
        virtual void cmdDraw(RHICommandBuffer* command_buffer,
                             uint32_t          vertex_count,
                             uint32_t          instance_count,
                             uint32_t          first_vertex,
                             uint32_t          first_instance)                                                               = 0;
        virtual void cmdDispatch(RHICommandBuffer* command_buffer,
                                 uint32_t          group_count_x,
                                 uint32_t          group_count_y,
                                 uint32_t          group_count_z)                                                            = 0;
        virtual void cmdDispatchIndirect(RHICommandBuffer* command_buffer, RHIBuffer* buffer, RHIDeviceSize offset) = 0;
        virtual void cmdPipelineBarrier(RHICommandBuffer*             command_buffer,
                                        RHIPipelineStageFlags         src_stage_mask,
                                        RHIPipelineStageFlags         dst_stage_mask,
                                        RHIDependencyFlags            dependency_flags,
                                        uint32_t                      memory_barrier_count,
                                        const RHIMemoryBarrier*       p_memory_barriers,
                                        uint32_t                      buffer_memory_barrier_count,
                                        const RHIBufferMemoryBarrier* p_buffer_memory_barriers,
                                        uint32_t                      image_memory_barrier_count,
                                        const RHIImageMemoryBarrier*  p_image_memory_barriers)                       = 0;
        virtual bool endCommandBuffer(RHICommandBuffer* command_buffer)                                             = 0;
        virtual void updateDescriptorSets(uint32_t                     descriptor_write_count,
                                          const RHIWriteDescriptorSet* p_descriptor_writes,
                                          uint32_t                     descriptor_copy_count,
                                          const RHICopyDescriptorSet*  p_descriptor_copies)                          = 0;
        virtual bool
        queueSubmit(RHIQueue* queue, uint32_t submit_count, const RHISubmitInfo* p_submits, RHIFence* fence) = 0;
        virtual bool queueWaitIdle(RHIQueue* queue)                                                          = 0;
        virtual void resetCommandPool()                                                                      = 0;
        virtual void waitForFences()                                                                         = 0;

        // query
        virtual void                     getPhysicalDeviceProperties(RHIPhysicalDeviceProperties* pProperties) = 0;
        virtual RHICommandBuffer*        getCurrentCommandBuffer() const                                       = 0;
        virtual RHICommandBuffer* const* getCommandBufferList() const                                          = 0;
        virtual RHICommandPool*          getCommandPoor() const                                                = 0;
        virtual RHIDescriptorPool*       getDescriptorPoor() const                                             = 0;
        virtual RHIFence* const*         getFenceList() const                                                  = 0;
        virtual QueueFamilyIndices       getQueueFamilyIndices() const                                         = 0;
        virtual RHIQueue*                getGraphicsQueue() const                                              = 0;
        virtual RHIQueue*                getComputeQueue() const                                               = 0;
        virtual RHISwapChainDesc         getSwapchainInfo()                                                    = 0;
        virtual RHIDepthImageDesc        getDepthImageInfo() const                                             = 0;
        virtual uint8_t                  getMaxFramesInFlight() const                                          = 0;
        virtual uint8_t                  getCurrentFrameIndex() const                                          = 0;
        virtual void                     setCurrentFrameIndex(uint8_t index)                                   = 0;

        // command write
        virtual RHICommandBuffer* beginSingleTimeCommands()                                                         = 0;
        virtual void              endSingleTimeCommands(RHICommandBuffer* command_buffer)                           = 0;
        virtual bool              prepareBeforePass(std::function<void()> pass_update_after_recreate_swapchain)     = 0;
        virtual void              submitRendering(std::function<void()> pass_update_after_recreate_swapchain)       = 0;
        virtual void              pushEvent(RHICommandBuffer* commond_buffer, const char* name, const float* color) = 0;
        virtual void              popEvent(RHICommandBuffer* commond_buffer)                                        = 0;

        // destory
        virtual void clear()                                                 = 0;
        virtual void clearSwapchain()                                        = 0;
        virtual void destroyDefaultSampler(RHIDefaultSamplerType type)       = 0;
        virtual void destroyMipmappedSampler()                               = 0;
        virtual void destroyShaderModule(RHIShader* shader)                  = 0;
        virtual void destroySemaphore(RHISemaphore* semaphore)               = 0;
        virtual void destroySampler(RHISampler* sampler)                     = 0;
        virtual void destroyInstance(RHIInstance* instance)                  = 0;
        virtual void destroyImageView(RHIImageView* image_view)              = 0;
        virtual void destroyImage(RHIImage* image)                           = 0;
        virtual void destroyFramebuffer(RHIFramebuffer* framebuffer)         = 0;
        virtual void destroyFence(RHIFence* fence)                           = 0;
        virtual void destroyDevice()                                         = 0;
        virtual void destroyCommandPool(RHICommandPool* command_pool)        = 0;
        virtual void destroyBuffer(RHIBuffer*& buffer)                       = 0;
        virtual void freeCommandBuffers(RHICommandPool*   command_pool,
                                        uint32_t          command_buffer_count,
                                        RHICommandBuffer* p_command_buffers) = 0;

        // memory
        virtual void freeMemory(RHIDeviceMemory*& memory)             = 0;
        virtual bool mapMemory(RHIDeviceMemory*  memory,
                               RHIDeviceSize     offset,
                               RHIDeviceSize     size,
                               RHIMemoryMapFlags flags,
                               void**            pp_data)                        = 0;
        virtual void unmapMemory(RHIDeviceMemory* memory)             = 0;
        virtual void invalidateMappedMemoryRanges(void*            p_next,
                                                  RHIDeviceMemory* memory,
                                                  RHIDeviceSize    offset,
                                                  RHIDeviceSize    size) = 0;
        virtual void
        flushMappedMemoryRanges(void* p_next, RHIDeviceMemory* memory, RHIDeviceSize offset, RHIDeviceSize size) = 0;

    private:
    };

    inline RHI::~RHI() = default;
} // namespace Bocchi
