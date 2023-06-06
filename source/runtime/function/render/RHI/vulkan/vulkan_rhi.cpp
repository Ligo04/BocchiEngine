#include "runtime/function/render/RHI/vulkan/vulkan_rhi.h"
#include "runtime/function/render/RHI/vulkan/vulkan_rhi_resource.h"
#include "runtime/function/render/RHI/vulkan/vulkan_util.h"
#include "runtime/function/render/window_system.h"
#include "runtime/core/base/macro.h"

#include <set>

#undef max

namespace Bocchi
{
    VulkanRHI::~VulkanRHI()
    {
        // TODO
    }
    void VulkanRHI::initialize(RHIInitInfo initialize_info)
    {
        m_window = initialize_info.window_system->getWindow();


        auto window_size = initialize_info.window_system->getWindowsSize();
        m_scissor        = {{0, 0}, {(uint32_t)window_size[0], (uint32_t)window_size[1]}};
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
        createWindowSurface();
        // physical device
        pickPhysicsDevice();
        // logical device
        createLogicalDevice();
        // command pool
        createCommandPool();
        // command Buffers
        createCommandBuffers();
        // semaphore and fence
        createSyncPrimitive();
        // swap chain
        createSwapchain();
        // image view
        createSwapchainImageViews();
        // assert allocator
        createAssetAllocator();
        
    }
    void VulkanRHI::prepareContext()
    {

    }

    bool VulkanRHI::allocateCommandBuffers(const RHICommandBufferAllocateInfo* p_allocate_info,
                                           RHICommandBuffer*&                  p_command_buffers)
    {
        VkCommandBufferAllocateInfo vk_command_buffer_allocate_info {};
        vk_command_buffer_allocate_info.sType       = (VkStructureType)p_allocate_info->sType;
        vk_command_buffer_allocate_info.pNext       = (const void*)p_allocate_info->pNext;
        vk_command_buffer_allocate_info.commandPool = ((VulkanCommandPool*)p_allocate_info->commandPool)->getResource();
        vk_command_buffer_allocate_info.level       = (VkCommandBufferLevel)p_allocate_info->level;
        vk_command_buffer_allocate_info.commandBufferCount = p_allocate_info->commandBufferCount;

        VkCommandBuffer vk_command_buffer;
        p_command_buffers = new RHICommandBuffer();
        VkResult result   = vkAllocateCommandBuffers(m_device, &vk_command_buffer_allocate_info, &vk_command_buffer);
        ((VulkanCommandBuffer*)p_command_buffers)->setResource(vk_command_buffer);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("vkAllocateCommandBuffers failed!");
            return false;
        }
    }

    bool VulkanRHI::allocateDescriptorSets(const RHIDescriptorSetAllocateInfo* p_allocate_info,
                                           RHIDescriptorSet*&                  p_descriptor_sets)
    {
        // descript_set_layout
        int                                descriptor_set_layout_size = p_allocate_info->descriptorSetCount;
        std::vector<VkDescriptorSetLayout> vk_descriptor_set_layout_list(descriptor_set_layout_size);

        for (int i = 0; i < descriptor_set_layout_size; ++i)
        {
            const auto& rhi_descriptor_set_layout_element = p_allocate_info->pSetLayouts[i];
            auto&       vk_descriptor_set_layout_element  = vk_descriptor_set_layout_list[i];

            vk_descriptor_set_layout_element =
                ((VulkanDescriptorSetLayout*)rhi_descriptor_set_layout_element)->getResource();

            VulkanDescriptorSetLayout* test = ((VulkanDescriptorSetLayout*)rhi_descriptor_set_layout_element);
        }

        VkDescriptorSetAllocateInfo vk_descriptor_set_allocate_info {};
        vk_descriptor_set_allocate_info.sType = (VkStructureType)p_allocate_info->sType;
        vk_descriptor_set_allocate_info.pNext = (const void*)p_allocate_info->pNext;
        vk_descriptor_set_allocate_info.descriptorPool =
            ((VulkanDescriptorPool*)p_allocate_info->descriptorPool)->getResource();
        vk_descriptor_set_allocate_info.descriptorSetCount = p_allocate_info->descriptorSetCount;
        vk_descriptor_set_allocate_info.pSetLayouts        = vk_descriptor_set_layout_list.data();

        VkDescriptorSet vk_descriptor_set {};
        p_descriptor_sets = new VulkanDescriptorSet();
        VkResult result   = vkAllocateDescriptorSets(m_device, &vk_descriptor_set_allocate_info, &vk_descriptor_set);
        ((VulkanDescriptorSet*)p_descriptor_sets)->setResource(vk_descriptor_set);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("vkAllocateDescriptorSets failed!");
            return false;
        }
    }

    void VulkanRHI::recreateSwapchain()
    {
        int width  = 0;
        int height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);
        while (width==0||height==0)
        {
            glfwGetFramebufferSize(m_window, &width, &height);
            glfwWaitEvents();
        }

        VkResult res_wait_for_fences =
            _vkWaitForFences(m_device, k_max_frames_in_flight, m_is_frame_in_flight_fences, VK_TRUE, UINT64_MAX);
        if (VK_SUCCESS!=res_wait_for_fences)
        {
            LOG_ERROR("_vkWaitForFences Failed!");
            return;
        }

        destroyImageView(m_depth_image_view);
        vkDestroyImage(m_device, ((VulkanImage*)m_depth_image)->getResource(), nullptr);
        vkFreeMemory(m_device, m_depth_image_memory, nullptr);

        createSwapchain();
        createSwapchainImageViews();
        createFramebufferImageAndView();
    }

    RHISampler* VulkanRHI::getOrCreateDefaultSampler(RHIDefaultSamplerType type)
    {
	    switch ( type )
	    {
            case Default_Sampler_Linear:
                if (m_linear_sampler==nullptr)
                {
                    m_linear_sampler = new VulkanSampler();
                    ((VulkanSampler*)m_linear_sampler)
                        ->setResource(VulkanUtil::getOrCreateLinearSampler(m_physical_device, m_device));
                }
                return m_linear_sampler;

	    	case Default_Sampler_Nearest:
                if (m_nearest_sampler == nullptr)
                {
                    m_nearest_sampler = new VulkanSampler();
                    ((VulkanSampler*)m_nearest_sampler)
                        ->setResource(VulkanUtil::getOrCreateNearestSampler(m_physical_device, m_device));
                }
                return m_nearest_sampler;
                    default:
                return nullptr;
	    }
    }

    RHISampler* VulkanRHI::getOrCreateMipmapSampler(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            LOG_ERROR("width==0||height==0");
            return nullptr;
        }
        uint32_t mip_levels   = floor(log2(std::max(width, height))) + 1;
        auto     find_sampler = m_mipmap_sampler_map.find(mip_levels);

        if (find_sampler != m_mipmap_sampler_map.end())
        {
            return find_sampler->second;
        }
        else
        {
	        RHISampler* sampler;
	        sampler = new VulkanSampler();

            VkSampler vk_sampler = VulkanUtil::getOrCreateMipmapSampler(m_physical_device, m_device, width, height);

            ((VulkanSampler*)sampler)->setResource(vk_sampler);
           
            m_mipmap_sampler_map.insert(std::make_pair(mip_levels, sampler));

            return sampler;
        }
    }

    RHIShader* VulkanRHI::createShaderModule(const std::vector<unsigned char>& shader_code)
    {
        RHIShader*     shader    = new VulkanShader();
        VkShaderModule vk_shader = VulkanUtil::createShaderModule(m_device, shader_code);

        ((VulkanShader*)shader)->setResource(vk_shader);
        return shader;
    }

    void VulkanRHI::createBuffer(RHIDeviceSize          size,
                                 RHIBufferUsageFlags    usage,
                                 RHIMemoryPropertyFlags properties,
                                 RHIBuffer*&            buffer,
                                 RHIDeviceMemory*&      buffer_memory)
    {
        VkBuffer       vk_buffer;
        VkDeviceMemory vk_device_memory;

        VulkanUtil::createBuffer(m_physical_device, m_device, size, usage, properties, vk_buffer, vk_device_memory);

        buffer        = new VulkanBuffer();
        buffer_memory = new VulkanDeviceMemory();

        ((VulkanBuffer*)buffer)->setResource(vk_buffer);
        ((VulkanDeviceMemory*)buffer)->setResource(vk_device_memory);
    }

    void VulkanRHI::createBufferAndInitialize(RHIBufferUsageFlags    usage,
                                              RHIMemoryPropertyFlags properties,
                                              RHIBuffer*&            buffer,
                                              RHIDeviceMemory*&      buffer_memory,
                                              RHIDeviceSize          size,
                                              void*                  data,
                                              int                    datasize)
    {
        VkBuffer       vk_buffer;
        VkDeviceMemory vk_device_memory;

        VulkanUtil::createBufferAndInitialize(
            m_device, m_physical_device, usage, properties, &vk_buffer, &vk_device_memory, size, data, datasize);

        buffer        = new VulkanBuffer();
        buffer_memory = new VulkanDeviceMemory();
        ((VulkanBuffer*)buffer)->setResource(vk_buffer);
        ((VulkanDeviceMemory*)buffer_memory)->setResource(vk_device_memory);
    }

    bool VulkanRHI::createBufferVma(VmaAllocator                   allocator,
                                    const RHIBufferCreateInfo*     p_buffer_create_info,
                                    const VmaAllocationCreateInfo* p_allocation_create_info,
                                    RHIBuffer*&                    p_buffer,
                                    VmaAllocation*                 p_allocation,
                                    VmaAllocationInfo*             p_allocation_info)
    {
        VkBuffer           vk_buffer;
        VkBufferCreateInfo vk_buffer_create_info {};
        vk_buffer_create_info.sType                 = (VkStructureType)p_buffer_create_info->sType;
        vk_buffer_create_info.pNext                 = (const void*)p_buffer_create_info->pNext;
        vk_buffer_create_info.flags                 = (VkBufferCreateFlags)p_buffer_create_info->flags;
        vk_buffer_create_info.size                  = (VkDeviceSize)p_buffer_create_info->size;
        vk_buffer_create_info.usage                 = (VkBufferUsageFlags)p_buffer_create_info->usage;
        vk_buffer_create_info.sharingMode           = (VkSharingMode)p_buffer_create_info->sharingMode;
        vk_buffer_create_info.queueFamilyIndexCount = p_buffer_create_info->queueFamilyIndexCount;
        vk_buffer_create_info.pQueueFamilyIndices   = (const uint32_t*)p_buffer_create_info->pQueueFamilyIndices;

        p_buffer        = new VulkanBuffer();
        VkResult result = vmaCreateBuffer(
            allocator, &vk_buffer_create_info, p_allocation_create_info, &vk_buffer, p_allocation, p_allocation_info);

        ((VulkanBuffer*)p_buffer)->setResource(vk_buffer);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        return false;
    }

    bool VulkanRHI::createBufferWithAlignmentVma(VmaAllocator allocator,
	    const RHIBufferCreateInfo* p_buffer_create_info,
	    const VmaAllocationCreateInfo* p_allocation_create_info,
	    RHIDeviceSize min_alignment,
	    RHIBuffer*& p_buffer,
	    VmaAllocation* p_allocation,
	    VmaAllocationInfo* p_allocation_info
    )
    {
        VkBuffer           vk_buffer;
        VkBufferCreateInfo vk_buffer_create_info {};
        vk_buffer_create_info.sType                 = (VkStructureType)p_buffer_create_info->sType;
        vk_buffer_create_info.pNext                 = (const void*)p_buffer_create_info->pNext;
        vk_buffer_create_info.flags                 = (VkBufferCreateFlags)p_buffer_create_info->flags;
        vk_buffer_create_info.size                  = (VkDeviceSize)p_buffer_create_info->size;
        vk_buffer_create_info.usage                 = (VkBufferUsageFlags)p_buffer_create_info->usage;
        vk_buffer_create_info.sharingMode           = (VkSharingMode)p_buffer_create_info->sharingMode;
        vk_buffer_create_info.queueFamilyIndexCount = p_buffer_create_info->queueFamilyIndexCount;
        vk_buffer_create_info.pQueueFamilyIndices   = (const uint32_t*)p_buffer_create_info->pQueueFamilyIndices;

        p_buffer        = new VulkanBuffer();
        VkResult result = vmaCreateBufferWithAlignment(
            allocator, &vk_buffer_create_info, p_allocation_create_info,min_alignment, &vk_buffer, p_allocation, p_allocation_info);

        ((VulkanBuffer*)p_buffer)->setResource(vk_buffer);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        return false;
    }

    void VulkanRHI::copyBuffer(RHIBuffer* src_buffer,
	    RHIBuffer* dst_buffer,
	    RHIDeviceSize src_offset,
	    RHIDeviceSize dst_offset,
	    RHIDeviceSize size
    )
    {
        VkBuffer vk_src_buffer = ((VulkanBuffer*)src_buffer)->getResource();
        VkBuffer vk_dst_buffer = ((VulkanBuffer*)dst_buffer)->getResource();

        VulkanUtil::copyBuffer(this, vk_src_buffer, vk_dst_buffer, src_offset, dst_offset, size);
    }

    void VulkanRHI::createImage(uint32_t               image_width,
                                uint32_t               image_height,
                                RHIFormat              format,
                                RHIImageTiling         image_tiling,
                                RHIImageUsageFlags     image_usage_flags,
                                RHIMemoryPropertyFlags memory_property_flags,
                                RHIImage*&             image,
                                RHIDeviceMemory*&      memory,
                                RHIImageCreateFlags    image_create_flags,
                                uint32_t               array_layers,
                                uint32_t               miplevels)
    {
        VkImage        vk_image;
        VkDeviceMemory vk_device_memory;
        VulkanUtil::createImage(m_physical_device,
                                m_device,
                                image_width,
                                image_height,
                                (VkFormat)format,
                                (VkImageTiling)image_tiling,
                                (VkImageUsageFlags)image_usage_flags,
                                (VkMemoryPropertyFlags)memory_property_flags,
                                vk_image,
                                vk_device_memory,
                                (VkImageCreateFlags)image_create_flags,
                                array_layers,
                                miplevels);

        image  = new VulkanImage();
        memory = new VulkanDeviceMemory();
        ((VulkanImage*)image)->setResource(vk_image);
        ((VulkanDeviceMemory*)memory)->setResource(vk_device_memory);
    }

    void VulkanRHI::createImageView(RHIImage*           image,
                                    RHIFormat           format,
                                    RHIImageAspectFlags image_aspect_flags,
                                    RHIImageViewType    view_type,
                                    uint32_t            layout_count,
                                    uint32_t            miplevels,
                                    RHIImageView*&      image_view)
    {
        image_view           = new VulkanImageView();
        VkImage     vk_image = ((VulkanImage*)image)->getResource();
        VkImageView vk_image_view;
        vk_image_view = VulkanUtil::createImageView(m_device,
                                                    vk_image,
                                                    (VkFormat)format,
                                                    image_aspect_flags,
                                                    (VkImageViewType)view_type,
                                                    layout_count,
                                                    miplevels);
        ((VulkanImageView*)image_view)->setResource(vk_image_view);
    }

    void VulkanRHI::createGlobalImage(RHIImage*& image,
	    RHIImageView*& image_view,
	    VmaAllocation& image_allocation,
	    uint32_t texture_image_width,
	    uint32_t texture_image_height,
	    void* texture_image_pixels,
	    RHIFormat texture_image_format,
	    uint32_t miplevels
    )
    {
        VkImage     vk_image;
        VkImageView vk_image_view;

        VulkanUtil::createGlobalImage(this,
                                      vk_image,
                                      vk_image_view,
                                      image_allocation,
                                      texture_image_width,
                                      texture_image_height,
                                      texture_image_pixels,
                                      texture_image_format,
                                      miplevels);

        image      = new VulkanImage();
        image_view = new VulkanImageView();
        ((VulkanImage*)image)->setResource(vk_image);
        ((VulkanImageView*)image_view)->setResource(vk_image_view);
    }

    void VulkanRHI::createCubeMap(RHIImage*& image,
	    RHIImageView*& image_view,
	    VmaAllocation& image_allocation,
	    uint32_t texture_image_width,
	    uint32_t texture_image_height,
	    std::array<void*, 6> texture_image_pixels,
	    RHIFormat texture_image_format,
	    uint32_t miplevels
    )
    {
        VkImage     vk_image;
        VkImageView vk_image_view;

        VulkanUtil::createCubeMap(this,
                                  vk_image,
                                  vk_image_view,
                                  image_allocation,
                                  texture_image_width,
                                  texture_image_height,
                                  texture_image_pixels,
                                  texture_image_format,
                                  miplevels);

        image      = new VulkanImage();
        image_view = new VulkanImageView();
        ((VulkanImage*)image)->setResource(vk_image);
        ((VulkanImageView*)image_view)->setResource(vk_image_view);
    }

    bool VulkanRHI::createCommandPool(const RHICommandPoolCreateInfo* p_create_info,
	    RHICommandPool*& p_command_pool
    )
    {
        VkCommandPoolCreateInfo create_info {};
        create_info.sType            = (VkStructureType)p_create_info->sType;
        create_info.pNext            = (const void*)p_create_info->pNext;
        create_info.flags            = (VkCommandPoolCreateFlags)p_create_info->flags;
        create_info.queueFamilyIndex = p_create_info->queueFamilyIndex;

        p_command_pool = new VulkanCommandPool();
        VkCommandPool vk_command_pool;
        VkResult      result = vkCreateCommandPool(m_device, &create_info, nullptr, &vk_command_pool);
        ((VulkanCommandPool*)p_create_info)->setResource(vk_command_pool);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreateCommandPool is failed!");
            return false;
        }
    }

    bool VulkanRHI::createDescriptorPool(const RHIDescriptorPoolCreateInfo* p_create_info,
                                         RHIDescriptorPool*&                p_descriptor_pool)
    {
        int                               size = p_create_info->poolSizeCount;
        std::vector<VkDescriptorPoolSize> descriptor_pool_size(size);
        for (int i = 0; i < size; ++i)
        {
            const auto& rhi_desc = p_create_info->pPoolSizes[i];
            auto&       vk_desc  = descriptor_pool_size[i];

            vk_desc.type            = (VkDescriptorType)rhi_desc.type;
            vk_desc.descriptorCount = rhi_desc.descriptorCount;
        };

        VkDescriptorPoolCreateInfo create_info {};
        create_info.sType         = (VkStructureType)p_create_info->sType;
        create_info.pNext         = (const void*)p_create_info->pNext;
        create_info.flags         = (VkDescriptorPoolCreateFlags)p_create_info->flags;
        create_info.maxSets       = p_create_info->maxSets;
        create_info.poolSizeCount = p_create_info->poolSizeCount;
        create_info.pPoolSizes    = descriptor_pool_size.data();

        p_descriptor_pool = new VulkanDescriptorPool();
        VkDescriptorPool vk_descriptor_pool;
        VkResult         result = vkCreateDescriptorPool(m_device, &create_info, nullptr, &vk_descriptor_pool);
        ((VulkanDescriptorPool*)p_descriptor_pool)->setResource(vk_descriptor_pool);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreateDescriptorPool is failed!");
            return false;
        }
    }

    bool VulkanRHI::createDescriptorSetLayout(const RHIDescriptorSetLayoutCreateInfo* p_create_info,
                                              RHIDescriptorSetLayout*&                p_set_layout)
    {
        int                                       descriptor_set_layout_binding_size = p_create_info->bindingCount;
        std::vector<VkDescriptorSetLayoutBinding> vk_descriptor_set_layout_bindings_list(
            descriptor_set_layout_binding_size);

        int sampler_count = 0;
        for (int i = 0; i < descriptor_set_layout_binding_size; ++i)
        {
            const auto& rhi_descriptor_set_layout_bingding_element = p_create_info->pBindings[i];
            if (rhi_descriptor_set_layout_bingding_element.pImmutableSamplers != nullptr)
            {
                sampler_count += rhi_descriptor_set_layout_bingding_element.descriptorCount;
            }
        }

        std::vector<VkSampler> sampler_list(sampler_count);
        int                    sampler_current = 0;

        for (int i = 0; i < descriptor_set_layout_binding_size; ++i)
        {
            const auto& rhi_descriptor_set_layout_bingding_element = p_create_info->pBindings[i];
            auto&       vk_descriptor_set_layout_binding_element   = vk_descriptor_set_layout_bindings_list[i];

            // sampler
            vk_descriptor_set_layout_binding_element.pImmutableSamplers = nullptr;
            for (int i = 0; rhi_descriptor_set_layout_bingding_element.pImmutableSamplers; ++i)
            {
                vk_descriptor_set_layout_binding_element.pImmutableSamplers = &sampler_list[sampler_current];
                for (int i = 0; i < rhi_descriptor_set_layout_bingding_element.descriptorCount; ++i)
                {
                    const auto& rhi_sampler_element = rhi_descriptor_set_layout_bingding_element.pImmutableSamplers[i];
                    auto&       vk_sampler_element  = sampler_list[sampler_current];

                    vk_sampler_element = ((VulkanSampler*)rhi_sampler_element)->getResource();

                    sampler_current++;
                }
            }

            vk_descriptor_set_layout_binding_element.binding = rhi_descriptor_set_layout_bingding_element.binding;
            vk_descriptor_set_layout_binding_element.descriptorType =
                (VkDescriptorType)rhi_descriptor_set_layout_bingding_element.descriptorType;
            vk_descriptor_set_layout_binding_element.descriptorCount =
                rhi_descriptor_set_layout_bingding_element.descriptorCount;
            vk_descriptor_set_layout_binding_element.stageFlags = rhi_descriptor_set_layout_bingding_element.stageFlags;

            if (sampler_count != sampler_current)
            {
                LOG_ERROR("sampler_count != sampller_current");
                return false;
            }

            VkDescriptorSetLayoutCreateInfo vk_descriptor_set_layout_create_info {};
            vk_descriptor_set_layout_create_info.sType        = (VkStructureType)p_create_info->sType;
            vk_descriptor_set_layout_create_info.pNext        = (const void*)p_create_info->pNext;
            vk_descriptor_set_layout_create_info.flags        = (VkDescriptorSetLayoutCreateFlags)p_create_info->flags;
            vk_descriptor_set_layout_create_info.bindingCount = p_create_info->bindingCount;
            vk_descriptor_set_layout_create_info.pBindings    = vk_descriptor_set_layout_bindings_list.data();

            p_set_layout = new VulkanDescriptorSetLayout();
            VkDescriptorSetLayout vk_descriptor_set_layout;
            VkResult              result = vkCreateDescriptorSetLayout(
                m_device, &vk_descriptor_set_layout_create_info, nullptr, &vk_descriptor_set_layout);
            ((VulkanDescriptorSetLayout*)p_set_layout)->setResource(vk_descriptor_set_layout);

            if (result == VK_SUCCESS)
            {
                return RHI_SUCCESS;
            }
            else
            {
                LOG_ERROR("vkCreateDescriptorSetLayout failed!");
                return false;
            }
        }
    }

    bool VulkanRHI::createFence(const RHIFenceCreateInfo* p_create_info, RHIFence*& p_fence)
    {
        VkFenceCreateInfo create_info {};
        create_info.sType = (VkStructureType)p_create_info->sType;
        create_info.pNext = (const void*)p_create_info->pNext;
        create_info.flags = (VkFenceCreateFlags)p_create_info->flags;

        p_fence = new VulkanFence();
        VkFence  vk_fence;
        VkResult result = vkCreateFence(m_device, &create_info, nullptr, &vk_fence);
        ((VulkanFence*)p_fence)->setResource(vk_fence);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreateFence failed!");
            return false;
        }
    }

    bool VulkanRHI::createFramebuffer(const RHIFramebufferCreateInfo* p_create_info, RHIFramebuffer*& p_framebuffer)
    {
        // image_view
        int                      image_view_size = p_create_info->attachmentCount;
        std::vector<VkImageView> vk_image_view_list(image_view_size);
        for (int i = 0; i < image_view_size; ++i)
        {
            const auto& rhi_image_view_element = p_create_info->pAttachments[i];
            auto&       vk_image_view_element  = vk_image_view_list[i];

            vk_image_view_element = ((VulkanImageView*)rhi_image_view_element)->getResource();
        };

        VkFramebufferCreateInfo create_info {};
        create_info.sType           = (VkStructureType)p_create_info->sType;
        create_info.pNext           = (const void*)p_create_info->pNext;
        create_info.flags           = (VkFramebufferCreateFlags)p_create_info->flags;
        create_info.renderPass      = ((VulkanRenderPass*)p_create_info->renderPass)->getResource();
        create_info.attachmentCount = p_create_info->attachmentCount;
        create_info.pAttachments    = vk_image_view_list.data();
        create_info.width           = p_create_info->width;
        create_info.height          = p_create_info->height;
        create_info.layers          = p_create_info->layers;

        p_framebuffer = new VulkanFramebuffer();
        VkFramebuffer vk_framebuffer;
        VkResult      result = vkCreateFramebuffer(m_device, &create_info, nullptr, &vk_framebuffer);
        ((VulkanFramebuffer*)p_framebuffer)->setResource(vk_framebuffer);

        if (result != VK_SUCCESS)
        {
	        LOG_ERROR("vkCreateFramebuffer failed!");
	        return false;
        }
        return RHI_SUCCESS;
    }

    bool VulkanRHI::createGraphicsPipelines(RHIPipelineCache*                    pipeline_cache,
                                            uint32_t                             create_info_count,
                                            const RHIGraphicsPipelineCreateInfo* p_create_info,
                                            RHIPipeline*&                        p_pipelines)
    {
        // pipeline_shader_stage_creat_info
        int                                          pileline_shaer_stage_create_info_size = p_create_info->stageCount;
        std::vector<VkPipelineShaderStageCreateInfo> vk_pipeline_shader_stage_create_info_list(
            pileline_shaer_stage_create_info_size);

        int specialization_map_entry_size_total = 0;
        int specialization_info_total           = 0;
        for (int i = 0; i < pileline_shaer_stage_create_info_size; ++i)
        {
            const auto& rhi_pipeline_share_stage_create_info_element = p_create_info->pStages[i];
            if (rhi_pipeline_share_stage_create_info_element.pSpecializationInfo != nullptr)
            {
                specialization_info_total++;
                specialization_map_entry_size_total +=
                    rhi_pipeline_share_stage_create_info_element.pSpecializationInfo->mapEntryCount;
            }
        }

        std::vector<VkSpecializationInfo>     vk_specialization_info_list(specialization_info_total);
        std::vector<VkSpecializationMapEntry> vk_specialization_map_entry_list(specialization_map_entry_size_total);
        int                                   specialization_map_entry_current = 0;
        int                                   specialization_info_current      = 0;

        for (int i = 0; i < pileline_shaer_stage_create_info_size; ++i)
        {
            const auto& rhi_pipeline_shader_stage_create_info_element = p_create_info->pStages[i];
            auto&       vk_pipeline_shader_stage_create_info_element  = vk_pipeline_shader_stage_create_info_list[i];

            if (rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo != nullptr)
            {
                vk_pipeline_shader_stage_create_info_element.pSpecializationInfo =
                    &vk_specialization_info_list[specialization_info_current];

                VkSpecializationInfo vk_specialization_info {};
                vk_specialization_info.mapEntryCount =
                    rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->mapEntryCount;
                vk_specialization_info.pMapEntries =
                    &vk_specialization_map_entry_list[specialization_map_entry_current];
                vk_specialization_info.dataSize =
                    rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->dataSize;
                vk_specialization_info.pData =
                    (const void*)rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->pData;

                for (int i = 0; i < rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->mapEntryCount;
                     ++i)
                {
                    const auto& rhi_specialization_map_entry_element =
                        rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->pMapEntries[i];
                    auto& vk_specialization_map_entry_element =
                        vk_specialization_map_entry_list[specialization_map_entry_current];

                    vk_specialization_map_entry_element.constantID = rhi_specialization_map_entry_element->constantID;
                    vk_specialization_map_entry_element.offset     = rhi_specialization_map_entry_element->offset;
                    vk_specialization_map_entry_element.size       = rhi_specialization_map_entry_element->size;

                    specialization_map_entry_current++;
                }

                specialization_info_current++;
            }
            else
            {
                vk_pipeline_shader_stage_create_info_element.pSpecializationInfo = nullptr;
            }

            vk_pipeline_shader_stage_create_info_element.sType =
                (VkStructureType)rhi_pipeline_shader_stage_create_info_element.sType;
            vk_pipeline_shader_stage_create_info_element.pNext =
                (const void*)rhi_pipeline_shader_stage_create_info_element.pNext;
            vk_pipeline_shader_stage_create_info_element.flags =
                (VkPipelineShaderStageCreateFlags)rhi_pipeline_shader_stage_create_info_element.flags;
            vk_pipeline_shader_stage_create_info_element.stage =
                (VkShaderStageFlagBits)rhi_pipeline_shader_stage_create_info_element.stage;
            vk_pipeline_shader_stage_create_info_element.module =
                ((VulkanShader*)rhi_pipeline_shader_stage_create_info_element.module)->getResource();
            vk_pipeline_shader_stage_create_info_element.pName = rhi_pipeline_shader_stage_create_info_element.pName;
        }

        if (!((specialization_map_entry_size_total == specialization_map_entry_current) &&
              (specialization_info_total == specialization_info_current)))
        {
            LOG_ERROR("(specialization_map_entry_size_total == specialization_map_entry_current)&& "
                      "(specialization_info_total == specialization_info_current)");
            return false;
        }

        // vertex_input_binding_description
        int vertex_input_binding_description_size = p_create_info->pVertexInputState->vertexBindingDescriptionCount;
        std::vector<VkVertexInputBindingDescription> vk_vertex_input_binding_description_list(
            vertex_input_binding_description_size);

        for (int i = 0; i < vertex_input_binding_description_size; ++i)
        {
            const auto& rhi_vertex_input_binding_description_element =
                p_create_info->pVertexInputState->pVertexBindingDescriptions[i];
            auto& vk_vertex_input_binding_description_element = vk_vertex_input_binding_description_list[i];

            vk_vertex_input_binding_description_element.binding = rhi_vertex_input_binding_description_element.binding;
            vk_vertex_input_binding_description_element.stride  = rhi_vertex_input_binding_description_element.stride;
            vk_vertex_input_binding_description_element.inputRate =
                (VkVertexInputRate)rhi_vertex_input_binding_description_element.inputRate;
        }

        // vertex_input_attribute_description
        int vertex_input_attribute_description_size = p_create_info->pVertexInputState->vertexAttributeDescriptionCount;
        std::vector<VkVertexInputAttributeDescription> vk_vertex_input_attribute_description_list(
            vertex_input_attribute_description_size);
        for (int i = 0; i < vertex_input_attribute_description_size; ++i)
        {
            const auto& rhi_vertex_input_attribute_description_element =
                p_create_info->pVertexInputState->pVertexAttributeDescriptions[i];
            auto& vk_vertex_input_attribute_description_element = vk_vertex_input_attribute_description_list[i];

            vk_vertex_input_attribute_description_element.location =
                rhi_vertex_input_attribute_description_element.location;
            vk_vertex_input_attribute_description_element.binding =
                rhi_vertex_input_attribute_description_element.binding;
            vk_vertex_input_attribute_description_element.format =
                (VkFormat)rhi_vertex_input_attribute_description_element.format;
            vk_vertex_input_attribute_description_element.offset =
                rhi_vertex_input_attribute_description_element.offset;
        };

        VkPipelineVertexInputStateCreateInfo vk_pipeline_vertex_input_state_create_info {};
        vk_pipeline_vertex_input_state_create_info.sType = (VkStructureType)p_create_info->pVertexInputState->sType;
        vk_pipeline_vertex_input_state_create_info.pNext = (const void*)p_create_info->pVertexInputState->pNext;
        vk_pipeline_vertex_input_state_create_info.flags =
            (VkPipelineVertexInputStateCreateFlags)p_create_info->pVertexInputState->flags;
        vk_pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount =
            p_create_info->pVertexInputState->vertexBindingDescriptionCount;
        vk_pipeline_vertex_input_state_create_info.pVertexBindingDescriptions =
            vk_vertex_input_binding_description_list.data();
        vk_pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount =
            p_create_info->pVertexInputState->vertexAttributeDescriptionCount;
        vk_pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions =
            vk_vertex_input_attribute_description_list.data();

        VkPipelineInputAssemblyStateCreateInfo vk_pipeline_input_assembly_state_create_info {};
        vk_pipeline_input_assembly_state_create_info.sType = (VkStructureType)p_create_info->pInputAssemblyState->sType;
        vk_pipeline_input_assembly_state_create_info.pNext = (const void*)p_create_info->pInputAssemblyState->pNext;
        vk_pipeline_input_assembly_state_create_info.flags =
            (VkPipelineCacheCreateFlags)p_create_info->pInputAssemblyState->flags;
        vk_pipeline_input_assembly_state_create_info.topology =
            (VkPrimitiveTopology)p_create_info->pInputAssemblyState->topology;
        vk_pipeline_input_assembly_state_create_info.primitiveRestartEnable =
            (VkBool32)p_create_info->pInputAssemblyState->primitiveRestartEnable;

        const VkPipelineTessellationStateCreateInfo* vk_pipeline_tessellation_state_create_info_ptr = nullptr;
        VkPipelineTessellationStateCreateInfo        vk_pipeline_tessellation_state_create_info {};

        if (p_create_info->pTessellationState != nullptr)
        {
            vk_pipeline_tessellation_state_create_info.sType =
                (VkStructureType)p_create_info->pTessellationState->sType;
            vk_pipeline_tessellation_state_create_info.pNext = (const void*)p_create_info->pTessellationState->pNext;
            vk_pipeline_tessellation_state_create_info.flags =
                (VkPipelineTessellationStateCreateFlags)p_create_info->pTessellationState->flags;
            vk_pipeline_tessellation_state_create_info.patchControlPoints =
                p_create_info->pTessellationState->patchControlPoints;

            vk_pipeline_tessellation_state_create_info_ptr = &vk_pipeline_tessellation_state_create_info;
        }

        // viewport
        int                     viewport_size = p_create_info->pViewportState->viewportCount;
        std::vector<VkViewport> vk_viewport_list(viewport_size);
        for (int i = 0; i < viewport_size; ++i)
        {
            const auto& rhi_viewport_element = p_create_info->pViewportState->pViewports[i];
            auto&       vk_viewport_element  = vk_viewport_list[i];
            vk_viewport_element.x            = rhi_viewport_element.x;
            vk_viewport_element.y            = rhi_viewport_element.y;
            vk_viewport_element.width        = rhi_viewport_element.width;
            vk_viewport_element.height       = rhi_viewport_element.height;
            vk_viewport_element.minDepth     = rhi_viewport_element.minDepth;
            vk_viewport_element.maxDepth     = rhi_viewport_element.maxDepth;
        }

        // rect_2d
        int                   rect_2d_size = p_create_info->pViewportState->scissorCount;
        std::vector<VkRect2D> vk_rect_2d_list(rect_2d_size);
        for (int i = 0; i < rect_2d_size; ++i)
        {
            const auto& rhi_rect_2d_element = p_create_info->pViewportState->pScissors[i];
            auto&       vk_rect_2d_element  = vk_rect_2d_list[i];

            VkOffset2D offset_2d {};
            offset_2d.x = rhi_rect_2d_element.offset.x;
            offset_2d.y = rhi_rect_2d_element.offset.y;

            VkExtent2D extent_2d {};
            extent_2d.width  = rhi_rect_2d_element.extent.width;
            extent_2d.height = rhi_rect_2d_element.extent.height;

            vk_rect_2d_element.offset = offset_2d;
            vk_rect_2d_element.extent = extent_2d;
        }

        VkPipelineViewportStateCreateInfo vk_pipeline_viewport_state_create_info {};
        vk_pipeline_viewport_state_create_info.sType = (VkStructureType)p_create_info->pViewportState->sType;
        vk_pipeline_viewport_state_create_info.pNext = (const void*)p_create_info->pViewportState->pNext;
        vk_pipeline_viewport_state_create_info.flags =
            (VkPipelineViewportStateCreateFlags)p_create_info->pViewportState->flags;
        vk_pipeline_viewport_state_create_info.viewportCount = p_create_info->pViewportState->viewportCount;
        vk_pipeline_viewport_state_create_info.pViewports    = vk_viewport_list.data();
        vk_pipeline_viewport_state_create_info.scissorCount  = p_create_info->pViewportState->scissorCount;
        vk_pipeline_viewport_state_create_info.pScissors     = vk_rect_2d_list.data();

        VkPipelineRasterizationStateCreateInfo vk_pipeline_rasterization_state_create_info {};
        vk_pipeline_rasterization_state_create_info.sType = (VkStructureType)p_create_info->pRasterizationState->sType;
        vk_pipeline_rasterization_state_create_info.pNext = (const void*)p_create_info->pRasterizationState->pNext;
        vk_pipeline_rasterization_state_create_info.flags =
            (VkPipelineRasterizationStateCreateFlags)p_create_info->pRasterizationState->flags;
        vk_pipeline_rasterization_state_create_info.depthClampEnable =
            (VkBool32)p_create_info->pRasterizationState->depthClampEnable;
        vk_pipeline_rasterization_state_create_info.rasterizerDiscardEnable =
            (VkBool32)p_create_info->pRasterizationState->rasterizerDiscardEnable;
        vk_pipeline_rasterization_state_create_info.polygonMode =
            (VkPolygonMode)p_create_info->pRasterizationState->polygonMode;
        vk_pipeline_rasterization_state_create_info.cullMode =
            (VkCullModeFlags)p_create_info->pRasterizationState->cullMode;
        vk_pipeline_rasterization_state_create_info.frontFace =
            (VkFrontFace)p_create_info->pRasterizationState->frontFace;
        vk_pipeline_rasterization_state_create_info.depthBiasEnable =
            (VkBool32)p_create_info->pRasterizationState->depthBiasEnable;
        vk_pipeline_rasterization_state_create_info.depthBiasConstantFactor =
            p_create_info->pRasterizationState->depthBiasConstantFactor;
        vk_pipeline_rasterization_state_create_info.depthBiasClamp = p_create_info->pRasterizationState->depthBiasClamp;
        vk_pipeline_rasterization_state_create_info.depthBiasSlopeFactor =
            p_create_info->pRasterizationState->depthBiasSlopeFactor;
        vk_pipeline_rasterization_state_create_info.lineWidth = p_create_info->pRasterizationState->lineWidth;

        VkPipelineMultisampleStateCreateInfo vk_pipeline_multisample_state_create_info {};
        vk_pipeline_multisample_state_create_info.sType = (VkStructureType)p_create_info->pMultisampleState->sType;
        vk_pipeline_multisample_state_create_info.pNext = (const void*)p_create_info->pMultisampleState->pNext;
        vk_pipeline_multisample_state_create_info.flags =
            (VkPipelineMultisampleStateCreateFlags)p_create_info->pMultisampleState->flags;
        vk_pipeline_multisample_state_create_info.rasterizationSamples =
            (VkSampleCountFlagBits)p_create_info->pMultisampleState->rasterizationSamples;
        vk_pipeline_multisample_state_create_info.sampleShadingEnable =
            (VkBool32)p_create_info->pMultisampleState->sampleShadingEnable;
        vk_pipeline_multisample_state_create_info.minSampleShading = p_create_info->pMultisampleState->minSampleShading;
        vk_pipeline_multisample_state_create_info.pSampleMask =
            (const RHISampleMask*)p_create_info->pMultisampleState->pSampleMask;
        vk_pipeline_multisample_state_create_info.alphaToCoverageEnable =
            (VkBool32)p_create_info->pMultisampleState->alphaToCoverageEnable;
        vk_pipeline_multisample_state_create_info.alphaToOneEnable =
            (VkBool32)p_create_info->pMultisampleState->alphaToOneEnable;

        VkStencilOpState stencil_op_state_front {};
        stencil_op_state_front.failOp      = (VkStencilOp)p_create_info->pDepthStencilState->front.failOp;
        stencil_op_state_front.passOp      = (VkStencilOp)p_create_info->pDepthStencilState->front.passOp;
        stencil_op_state_front.depthFailOp = (VkStencilOp)p_create_info->pDepthStencilState->front.depthFailOp;
        stencil_op_state_front.compareOp   = (VkCompareOp)p_create_info->pDepthStencilState->front.compareOp;
        stencil_op_state_front.compareMask = p_create_info->pDepthStencilState->front.compareMask;
        stencil_op_state_front.writeMask   = p_create_info->pDepthStencilState->front.writeMask;
        stencil_op_state_front.reference   = p_create_info->pDepthStencilState->front.reference;

        VkStencilOpState stencil_op_state_back {};
        stencil_op_state_back.failOp      = (VkStencilOp)p_create_info->pDepthStencilState->back.failOp;
        stencil_op_state_back.passOp      = (VkStencilOp)p_create_info->pDepthStencilState->back.passOp;
        stencil_op_state_back.depthFailOp = (VkStencilOp)p_create_info->pDepthStencilState->back.depthFailOp;
        stencil_op_state_back.compareOp   = (VkCompareOp)p_create_info->pDepthStencilState->back.compareOp;
        stencil_op_state_back.compareMask = p_create_info->pDepthStencilState->back.compareMask;
        stencil_op_state_back.writeMask   = p_create_info->pDepthStencilState->back.writeMask;
        stencil_op_state_back.reference   = p_create_info->pDepthStencilState->back.reference;

        VkPipelineDepthStencilStateCreateInfo vk_pipeline_depth_stencil_state_create_info {};
        vk_pipeline_depth_stencil_state_create_info.sType = (VkStructureType)p_create_info->pDepthStencilState->sType;
        vk_pipeline_depth_stencil_state_create_info.pNext = (const void*)p_create_info->pDepthStencilState->pNext;
        vk_pipeline_depth_stencil_state_create_info.flags =
            (VkPipelineDepthStencilStateCreateFlags)p_create_info->pDepthStencilState->flags;
        vk_pipeline_depth_stencil_state_create_info.depthTestEnable =
            (VkBool32)p_create_info->pDepthStencilState->depthTestEnable;
        vk_pipeline_depth_stencil_state_create_info.depthWriteEnable =
            (VkBool32)p_create_info->pDepthStencilState->depthWriteEnable;
        vk_pipeline_depth_stencil_state_create_info.depthCompareOp =
            (VkCompareOp)p_create_info->pDepthStencilState->depthCompareOp;
        vk_pipeline_depth_stencil_state_create_info.depthBoundsTestEnable =
            (VkBool32)p_create_info->pDepthStencilState->depthBoundsTestEnable;
        vk_pipeline_depth_stencil_state_create_info.stencilTestEnable =
            (VkBool32)p_create_info->pDepthStencilState->stencilTestEnable;
        vk_pipeline_depth_stencil_state_create_info.front          = stencil_op_state_front;
        vk_pipeline_depth_stencil_state_create_info.back           = stencil_op_state_back;
        vk_pipeline_depth_stencil_state_create_info.minDepthBounds = p_create_info->pDepthStencilState->minDepthBounds;
        vk_pipeline_depth_stencil_state_create_info.maxDepthBounds = p_create_info->pDepthStencilState->maxDepthBounds;

        // pipeline_color_blend_attachment_state
        int pipeline_color_blend_attachment_state_size = p_create_info->pColorBlendState->attachmentCount;
        std::vector<VkPipelineColorBlendAttachmentState> vk_pipeline_color_blend_attachment_state_list(
            pipeline_color_blend_attachment_state_size);
        for (int i = 0; i < pipeline_color_blend_attachment_state_size; ++i)
        {
            const auto& rhi_pipeline_color_blend_attachment_state_element =
                p_create_info->pColorBlendState->pAttachments[i];
            auto& vk_pipeline_color_blend_attachment_state_element = vk_pipeline_color_blend_attachment_state_list[i];

            vk_pipeline_color_blend_attachment_state_element.blendEnable =
                (VkBool32)rhi_pipeline_color_blend_attachment_state_element.blendEnable;
            vk_pipeline_color_blend_attachment_state_element.srcColorBlendFactor =
                (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.srcColorBlendFactor;
            vk_pipeline_color_blend_attachment_state_element.dstColorBlendFactor =
                (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.dstColorBlendFactor;
            vk_pipeline_color_blend_attachment_state_element.colorBlendOp =
                (VkBlendOp)rhi_pipeline_color_blend_attachment_state_element.colorBlendOp;
            vk_pipeline_color_blend_attachment_state_element.srcAlphaBlendFactor =
                (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.srcAlphaBlendFactor;
            vk_pipeline_color_blend_attachment_state_element.dstAlphaBlendFactor =
                (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.dstAlphaBlendFactor;
            vk_pipeline_color_blend_attachment_state_element.alphaBlendOp =
                (VkBlendOp)rhi_pipeline_color_blend_attachment_state_element.alphaBlendOp;
            vk_pipeline_color_blend_attachment_state_element.colorWriteMask =
                (VkColorComponentFlags)rhi_pipeline_color_blend_attachment_state_element.colorWriteMask;
        };

        VkPipelineColorBlendStateCreateInfo vk_pipeline_color_blend_state_create_info {};
        vk_pipeline_color_blend_state_create_info.sType = (VkStructureType)p_create_info->pColorBlendState->sType;
        vk_pipeline_color_blend_state_create_info.pNext = p_create_info->pColorBlendState->pNext;
        vk_pipeline_color_blend_state_create_info.flags = p_create_info->pColorBlendState->flags;
        vk_pipeline_color_blend_state_create_info.logicOpEnable   = p_create_info->pColorBlendState->logicOpEnable;
        vk_pipeline_color_blend_state_create_info.logicOp         = (VkLogicOp)p_create_info->pColorBlendState->logicOp;
        vk_pipeline_color_blend_state_create_info.attachmentCount = p_create_info->pColorBlendState->attachmentCount;
        vk_pipeline_color_blend_state_create_info.pAttachments = vk_pipeline_color_blend_attachment_state_list.data();
        for (int i = 0; i < 4; ++i)
        {
            vk_pipeline_color_blend_state_create_info.blendConstants[i] =
                p_create_info->pColorBlendState->blendConstants[i];
        };

        // dynamic_state
        int                         dynamic_state_size = p_create_info->pDynamicState->dynamicStateCount;
        std::vector<VkDynamicState> vk_dynamic_state_list(dynamic_state_size);
        for (int i = 0; i < dynamic_state_size; ++i)
        {
            const auto& rhi_dynamic_state_element = p_create_info->pDynamicState->pDynamicStates[i];
            auto&       vk_dynamic_state_element  = vk_dynamic_state_list[i];

            vk_dynamic_state_element = (VkDynamicState)rhi_dynamic_state_element;
        };

        VkPipelineDynamicStateCreateInfo vk_pipeline_dynamic_state_create_info {};
        vk_pipeline_dynamic_state_create_info.sType = (VkStructureType)p_create_info->pDynamicState->sType;
        vk_pipeline_dynamic_state_create_info.pNext = p_create_info->pDynamicState->pNext;
        vk_pipeline_dynamic_state_create_info.flags =
            (VkPipelineDynamicStateCreateFlags)p_create_info->pDynamicState->flags;
        vk_pipeline_dynamic_state_create_info.dynamicStateCount = p_create_info->pDynamicState->dynamicStateCount;
        vk_pipeline_dynamic_state_create_info.pDynamicStates    = vk_dynamic_state_list.data();

        VkGraphicsPipelineCreateInfo create_info {};
        create_info.sType               = (VkStructureType)p_create_info->sType;
        create_info.pNext               = (const void*)p_create_info->pNext;
        create_info.flags               = (VkPipelineCreateFlags)p_create_info->flags;
        create_info.stageCount          = p_create_info->stageCount;
        create_info.pStages             = vk_pipeline_shader_stage_create_info_list.data();
        create_info.pVertexInputState   = &vk_pipeline_vertex_input_state_create_info;
        create_info.pInputAssemblyState = &vk_pipeline_input_assembly_state_create_info;
        create_info.pTessellationState  = vk_pipeline_tessellation_state_create_info_ptr;
        create_info.pViewportState      = &vk_pipeline_viewport_state_create_info;
        create_info.pRasterizationState = &vk_pipeline_rasterization_state_create_info;
        create_info.pMultisampleState   = &vk_pipeline_multisample_state_create_info;
        create_info.pDepthStencilState  = &vk_pipeline_depth_stencil_state_create_info;
        create_info.pColorBlendState    = &vk_pipeline_color_blend_state_create_info;
        create_info.pDynamicState       = &vk_pipeline_dynamic_state_create_info;
        create_info.layout              = ((VulkanPipelineLayout*)p_create_info->layout)->getResource();
        create_info.renderPass          = ((VulkanRenderPass*)p_create_info->renderPass)->getResource();
        create_info.subpass             = p_create_info->subpass;
        if (p_create_info->basePipelineHandle != nullptr)
        {
            create_info.basePipelineHandle = ((VulkanPipeline*)p_create_info->basePipelineHandle)->getResource();
        }
        else
        {
            create_info.basePipelineHandle = VK_NULL_HANDLE;
        }
        create_info.basePipelineIndex = p_create_info->basePipelineIndex;

        p_pipelines = new VulkanPipeline();
        VkPipeline      vk_pipelines;
        VkPipelineCache vk_pipeline_cache = VK_NULL_HANDLE;
        if (p_pipelines != nullptr)
        {
            vk_pipeline_cache = ((VulkanPipelineCache*)p_pipelines)->getResource();
        }
        VkResult result = vkCreateGraphicsPipelines(
            m_device, vk_pipeline_cache, create_info_count, &create_info, nullptr, &vk_pipelines);
        ((VulkanPipeline*)p_pipelines)->setResource(vk_pipelines);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreateGraphicsPipelines failed!");
            return false;
        }
    }

    bool VulkanRHI::createComputePipelines(RHIPipelineCache*                   pipeline_cache,
                                           uint32_t                            create_info_count,
                                           const RHIComputePipelineCreateInfo* p_create_info,
                                           RHIPipeline*&                       p_pipelines)
    {
        VkPipelineShaderStageCreateInfo shader_stage_create_info {};
        if (p_create_info->pStages->pSpecializationInfo != nullptr)
        {
            // will be complete soon if needed.
            shader_stage_create_info.pSpecializationInfo = nullptr;
        }
        else
        {
            shader_stage_create_info.pSpecializationInfo = nullptr;
        }
        shader_stage_create_info.sType  = (VkStructureType)p_create_info->pStages->sType;
        shader_stage_create_info.pNext  = (const void*)p_create_info->pStages->pNext;
        shader_stage_create_info.flags  = (VkPipelineShaderStageCreateFlags)p_create_info->pStages->flags;
        shader_stage_create_info.stage  = (VkShaderStageFlagBits)p_create_info->pStages->stage;
        shader_stage_create_info.module = ((VulkanShader*)p_create_info->pStages->module)->getResource();
        shader_stage_create_info.pName  = p_create_info->pStages->pName;

        VkComputePipelineCreateInfo create_info {};
        create_info.sType  = (VkStructureType)p_create_info->sType;
        create_info.pNext  = (const void*)p_create_info->pNext;
        create_info.flags  = (VkPipelineCreateFlags)p_create_info->flags;
        create_info.stage  = shader_stage_create_info;
        create_info.layout = ((VulkanPipelineLayout*)p_create_info->layout)->getResource();
        ;
        if (p_create_info->basePipelineHandle != nullptr)
        {
            create_info.basePipelineHandle = ((VulkanPipeline*)p_create_info->basePipelineHandle)->getResource();
        }
        else
        {
            create_info.basePipelineHandle = VK_NULL_HANDLE;
        }
        create_info.basePipelineIndex = p_create_info->basePipelineIndex;

        p_pipelines = new VulkanPipeline();
        VkPipeline      vk_pipelines;
        VkPipelineCache vk_pipeline_cache = VK_NULL_HANDLE;
        if (pipeline_cache != nullptr)
        {
            vk_pipeline_cache = ((VulkanPipelineCache*)pipeline_cache)->getResource();
        }
        VkResult result = vkCreateComputePipelines(
            m_device, vk_pipeline_cache, create_info_count, &create_info, nullptr, &vk_pipelines);
        ((VulkanPipeline*)p_pipelines)->setResource(vk_pipelines);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreateComputePipelines failed!");
            return false;
        }
    }

    bool VulkanRHI::createPipelineLayout(const RHIPipelineLayoutCreateInfo* p_create_info,
                                         RHIPipelineLayout*&                p_pipeline_layout)
    {
        // descriptor_set_layout
        int                                descriptor_set_layout_size = p_create_info->setLayoutCount;
        std::vector<VkDescriptorSetLayout> vk_descriptor_set_layout_list(descriptor_set_layout_size);
        for (int i = 0; i < descriptor_set_layout_size; ++i)
        {
            const auto& rhi_descriptor_set_layout_element = p_create_info->pSetLayouts[i];
            auto&       vk_descriptor_set_layout_element  = vk_descriptor_set_layout_list[i];

            vk_descriptor_set_layout_element =
                ((VulkanDescriptorSetLayout*)rhi_descriptor_set_layout_element)->getResource();
        };

        VkPipelineLayoutCreateInfo create_info {};
        create_info.sType          = (VkStructureType)p_create_info->sType;
        create_info.pNext          = (const void*)p_create_info->pNext;
        create_info.flags          = (VkPipelineLayoutCreateFlags)p_create_info->flags;
        create_info.setLayoutCount = p_create_info->setLayoutCount;
        create_info.pSetLayouts    = vk_descriptor_set_layout_list.data();

        p_pipeline_layout = new VulkanPipelineLayout();
        VkPipelineLayout vk_pipeline_layout;
        VkResult         result = vkCreatePipelineLayout(m_device, &create_info, nullptr, &vk_pipeline_layout);
        ((VulkanPipelineLayout*)p_pipeline_layout)->setResource(vk_pipeline_layout);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreatePipelineLayout failed!");
            return false;
        }
    }

    bool VulkanRHI::createRenderPass(const RHIRenderPassCreateInfo* p_create_info,
	    RHIRenderPass*& p_render_pass
    )
    {
        // attachment convert
        std::vector<VkAttachmentDescription> vk_attachments(p_create_info->attachmentCount);
        for (int i = 0; i < p_create_info->attachmentCount; ++i)
        {
            const auto& rhi_desc = p_create_info->pAttachments[i];
            auto&       vk_desc  = vk_attachments[i];

            vk_desc.flags          = (VkAttachmentDescriptionFlags)(rhi_desc).flags;
            vk_desc.format         = (VkFormat)(rhi_desc).format;
            vk_desc.samples        = (VkSampleCountFlagBits)(rhi_desc).samples;
            vk_desc.loadOp         = (VkAttachmentLoadOp)(rhi_desc).loadOp;
            vk_desc.storeOp        = (VkAttachmentStoreOp)(rhi_desc).storeOp;
            vk_desc.stencilLoadOp  = (VkAttachmentLoadOp)(rhi_desc).stencilLoadOp;
            vk_desc.stencilStoreOp = (VkAttachmentStoreOp)(rhi_desc).stencilStoreOp;
            vk_desc.initialLayout  = (VkImageLayout)(rhi_desc).initialLayout;
            vk_desc.finalLayout    = (VkImageLayout)(rhi_desc).finalLayout;
        };

        //subpass convert
        int total_attachment_refenrence = 0;
        for (int i = 0; i < p_create_info->subpassCount; i++)
        {
            const auto& rhi_desc = p_create_info->pSubpasses[i];
            total_attachment_refenrence += rhi_desc.inputAttachmentCount; // pInputAttachments
            total_attachment_refenrence += rhi_desc.colorAttachmentCount; // pColorAttachments
            if (rhi_desc.pDepthStencilAttachment != nullptr)
            {
                total_attachment_refenrence += rhi_desc.colorAttachmentCount; // pDepthStencilAttachment
            }
            if (rhi_desc.pResolveAttachments != nullptr)
            {
                total_attachment_refenrence += rhi_desc.colorAttachmentCount; // pResolveAttachments
            }
        }
        std::vector<VkSubpassDescription>  vk_subpass_description(p_create_info->subpassCount);
        std::vector<VkAttachmentReference> vk_attachment_reference(total_attachment_refenrence);
        int                                current_attachment_refence = 0;
        for (int i = 0; i < p_create_info->subpassCount; ++i)
        {
            const auto& rhi_desc = p_create_info->pSubpasses[i];
            auto&       vk_desc  = vk_subpass_description[i];

            vk_desc.flags                   = (VkSubpassDescriptionFlags)(rhi_desc).flags;
            vk_desc.pipelineBindPoint       = (VkPipelineBindPoint)(rhi_desc).pipelineBindPoint;
            vk_desc.preserveAttachmentCount = (rhi_desc).preserveAttachmentCount;
            vk_desc.pPreserveAttachments    = (const uint32_t*)(rhi_desc).pPreserveAttachments;

            vk_desc.inputAttachmentCount = (rhi_desc).inputAttachmentCount;
            vk_desc.pInputAttachments    = &vk_attachment_reference[current_attachment_refence];
            for (int i = 0; i < (rhi_desc).inputAttachmentCount; i++)
            {
                const auto& rhi_attachment_refence_input = (rhi_desc).pInputAttachments[i];
                auto&       vk_attachment_refence_input  = vk_attachment_reference[current_attachment_refence];

                vk_attachment_refence_input.attachment = rhi_attachment_refence_input.attachment;
                vk_attachment_refence_input.layout     = (VkImageLayout)(rhi_attachment_refence_input.layout);

                current_attachment_refence += 1;
            };

            vk_desc.colorAttachmentCount = (rhi_desc).colorAttachmentCount;
            vk_desc.pColorAttachments    = &vk_attachment_reference[current_attachment_refence];
            for (int i = 0; i < (rhi_desc).colorAttachmentCount; ++i)
            {
                const auto& rhi_attachment_refence_color = (rhi_desc).pColorAttachments[i];
                auto&       vk_attachment_refence_color  = vk_attachment_reference[current_attachment_refence];

                vk_attachment_refence_color.attachment = rhi_attachment_refence_color.attachment;
                vk_attachment_refence_color.layout     = (VkImageLayout)(rhi_attachment_refence_color.layout);

                current_attachment_refence += 1;
            };

            if (rhi_desc.pResolveAttachments != nullptr)
            {
                vk_desc.pResolveAttachments = &vk_attachment_reference[current_attachment_refence];
                for (int i = 0; i < (rhi_desc).colorAttachmentCount; ++i)
                {
                    const auto& rhi_attachment_refence_resolve = (rhi_desc).pResolveAttachments[i];
                    auto&       vk_attachment_refence_resolve  = vk_attachment_reference[current_attachment_refence];

                    vk_attachment_refence_resolve.attachment = rhi_attachment_refence_resolve.attachment;
                    vk_attachment_refence_resolve.layout     = (VkImageLayout)(rhi_attachment_refence_resolve.layout);

                    current_attachment_refence += 1;
                };
            }

            if (rhi_desc.pDepthStencilAttachment != nullptr)
            {
                vk_desc.pDepthStencilAttachment = &vk_attachment_reference[current_attachment_refence];
                for (int i = 0; i < (rhi_desc).colorAttachmentCount; ++i)
                {
                    const auto& rhi_attachment_refence_depth = (rhi_desc).pDepthStencilAttachment[i];
                    auto&       vk_attachment_refence_depth  = vk_attachment_reference[current_attachment_refence];

                    vk_attachment_refence_depth.attachment = rhi_attachment_refence_depth.attachment;
                    vk_attachment_refence_depth.layout     = (VkImageLayout)(rhi_attachment_refence_depth.layout);

                    current_attachment_refence += 1;
                };
            };
        };
        if (current_attachment_refence != total_attachment_refenrence)
        {
            LOG_ERROR("currentAttachmentRefence != totalAttachmentRefenrence");
            return false;
        }

        std::vector<VkSubpassDependency> vk_subpass_depandecy(p_create_info->dependencyCount);
        for (int i = 0; i < p_create_info->dependencyCount; ++i)
        {
            const auto& rhi_desc = p_create_info->pDependencies[i];
            auto&       vk_desc  = vk_subpass_depandecy[i];

            vk_desc.srcSubpass      = rhi_desc.srcSubpass;
            vk_desc.dstSubpass      = rhi_desc.dstSubpass;
            vk_desc.srcStageMask    = (VkPipelineStageFlags)(rhi_desc).srcStageMask;
            vk_desc.dstStageMask    = (VkPipelineStageFlags)(rhi_desc).dstStageMask;
            vk_desc.srcAccessMask   = (VkAccessFlags)(rhi_desc).srcAccessMask;
            vk_desc.dstAccessMask   = (VkAccessFlags)(rhi_desc).dstAccessMask;
            vk_desc.dependencyFlags = (VkDependencyFlags)(rhi_desc).dependencyFlags;
        };

        VkRenderPassCreateInfo vk_render_pass_create_info {};
        vk_render_pass_create_info.sType           = (VkStructureType)p_create_info->sType;
        vk_render_pass_create_info.pNext           = (const void*)p_create_info->pNext;
        vk_render_pass_create_info.flags           = (VkRenderPassCreateFlags)p_create_info->flags;
        vk_render_pass_create_info.attachmentCount = p_create_info->attachmentCount;
        vk_render_pass_create_info.pAttachments    = vk_attachments.data();
        vk_render_pass_create_info.subpassCount    = p_create_info->subpassCount;
        vk_render_pass_create_info.pSubpasses      = vk_subpass_description.data();
        vk_render_pass_create_info.dependencyCount = p_create_info->dependencyCount;
        vk_render_pass_create_info.pDependencies   = vk_subpass_depandecy.data();

        p_render_pass = new VulkanRenderPass();
        VkRenderPass vk_render_pass;
        VkResult     result = vkCreateRenderPass(m_device, &vk_render_pass_create_info, nullptr, &vk_render_pass);
        ((VulkanRenderPass*)p_render_pass)->setResource(vk_render_pass);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreateRenderPass failed!");
            return false;
        }
    }

    bool VulkanRHI::createSampler(const RHISamplerCreateInfo* p_create_info,
	    RHISampler*& p_sampler
    )
    {
        VkSamplerCreateInfo create_info {};
        create_info.sType                   = (VkStructureType)p_create_info->sType;
        create_info.pNext                   = (const void*)p_create_info->pNext;
        create_info.flags                   = (VkSamplerCreateFlags)p_create_info->flags;
        create_info.magFilter               = (VkFilter)p_create_info->magFilter;
        create_info.minFilter               = (VkFilter)p_create_info->minFilter;
        create_info.mipmapMode              = (VkSamplerMipmapMode)p_create_info->mipmapMode;
        create_info.addressModeU            = (VkSamplerAddressMode)p_create_info->addressModeU;
        create_info.addressModeV            = (VkSamplerAddressMode)p_create_info->addressModeV;
        create_info.addressModeW            = (VkSamplerAddressMode)p_create_info->addressModeW;
        create_info.mipLodBias              = p_create_info->mipLodBias;
        create_info.anisotropyEnable        = (VkBool32)p_create_info->anisotropyEnable;
        create_info.maxAnisotropy           = p_create_info->maxAnisotropy;
        create_info.compareEnable           = (VkBool32)p_create_info->compareEnable;
        create_info.compareOp               = (VkCompareOp)p_create_info->compareOp;
        create_info.minLod                  = p_create_info->minLod;
        create_info.maxLod                  = p_create_info->maxLod;
        create_info.borderColor             = (VkBorderColor)p_create_info->borderColor;
        create_info.unnormalizedCoordinates = (VkBool32)p_create_info->unnormalizedCoordinates;

        p_sampler = new VulkanSampler();
        VkSampler vk_sampler;
        VkResult  result = vkCreateSampler(m_device, &create_info, nullptr, &vk_sampler);
        ((VulkanSampler*)p_sampler)->setResource(vk_sampler);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        LOG_ERROR("vkCreateSampler failed!");
        return false;
    }

    bool VulkanRHI::createSemaphore(const RHISemaphoreCreateInfo* p_create_info,
	    RHISemaphore*& p_semaphore
    )
    {
        VkSemaphoreCreateInfo create_info {};
        create_info.sType = (VkStructureType)p_create_info->sType;
        create_info.pNext = p_create_info->pNext;
        create_info.flags = (VkSemaphoreCreateFlags)p_create_info->flags;

        p_semaphore = new VulkanSemaphore();
        VkSemaphore vk_semaphore;
        VkResult    result = vkCreateSemaphore(m_device, &create_info, nullptr, &vk_semaphore);
        ((VulkanSemaphore*)p_semaphore)->setResource(vk_semaphore);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreateSemaphore failed!");
            return false;
        }
    }

    bool VulkanRHI::waitForFencesPFN(uint32_t fence_count,
	    RHIFence* const* p_fence,
	    RHIBool32 wait_all,
	    uint64_t timeout
    )
    {
	    //fence
        int fence_size = fence_count;
        std::vector<VkFence> vk_fence_list(fence_size);
        for (int i=0;i<fence_size;++i)
        {
            const auto& rhi_fence_element = p_fence[i];
            auto&       vk_fence_element  = vk_fence_list[i];

            vk_fence_element = ((VulkanFence*)rhi_fence_element)->getResource();
        }
        VkResult result = _vkWaitForFences(m_device, fence_count, vk_fence_list.data(), wait_all, timeout);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("_vkWaitForFences failed!");
            return false;
        }
    }

    bool VulkanRHI::resetFencesPFN(uint32_t fence_count, RHIFence* const* p_fences)
    {
        // fence
        int                  fence_size = fence_count;
        std::vector<VkFence> vk_fence_list(fence_size);
        for (int i = 0; i < fence_size; ++i)
        {
            const auto& rhi_fence_element = p_fences[i];
            auto&       vk_fence_element  = vk_fence_list[i];

            vk_fence_element = ((VulkanFence*)rhi_fence_element)->getResource();
        };

        VkResult result = _vkResetFences(m_device, fence_count, vk_fence_list.data());

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("_vkResetFences failed!");
            return false;
        }
    }

    bool VulkanRHI::resetCommandPoolPFN(RHICommandPool* command_pool, RHICommandPoolResetFlags flags)
    {
        VkResult result = _vkResetCommandPool(
            m_device, ((VulkanCommandPool*)command_pool)->getResource(), (VkCommandPoolResetFlags)flags);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("_vkResetCommandPool failed!");
            return false;
        }
    }

    bool VulkanRHI::beginCommandBufferPFN(RHICommandBuffer* command_buffer,
	    const RHICommandBufferBeginInfo* p_begin_info
    )
    {
        VkCommandBufferInheritanceInfo* command_buffer_inheritance_info_ptr = nullptr;
        VkCommandBufferInheritanceInfo  command_buffer_inheritance_info {};
        if (p_begin_info->pInheritanceInfo!=nullptr)
        {
            command_buffer_inheritance_info.sType = (VkStructureType)p_begin_info->pInheritanceInfo->sType;
            command_buffer_inheritance_info.pNext = (const void*)p_begin_info->pInheritanceInfo->pNext;
            command_buffer_inheritance_info.renderPass =
                ((VulkanRenderPass*)p_begin_info->pInheritanceInfo->renderPass)->getResource();
            command_buffer_inheritance_info.subpass = p_begin_info->pInheritanceInfo->subpass;
            command_buffer_inheritance_info.framebuffer =
                ((VulkanFramebuffer*)p_begin_info->pInheritanceInfo->framebuffer)->getResource();
            command_buffer_inheritance_info.occlusionQueryEnable =
                (VkBool32)p_begin_info->pInheritanceInfo->occlusionQueryEnable;
            command_buffer_inheritance_info.queryFlags =
                (VkQueryControlFlags)p_begin_info->pInheritanceInfo->queryFlags;
            command_buffer_inheritance_info.pipelineStatistics =
                (VkQueryPipelineStatisticFlags)p_begin_info->pInheritanceInfo->pipelineStatistics;

            command_buffer_inheritance_info_ptr = &command_buffer_inheritance_info;
        }

        VkCommandBufferBeginInfo vk_command_buffer_begin_info {};
        vk_command_buffer_begin_info.sType            = (VkStructureType)p_begin_info->sType;
        vk_command_buffer_begin_info.pNext            = (const void*)p_begin_info->pNext;
        vk_command_buffer_begin_info.flags            = (VkCommandBufferUsageFlags)p_begin_info->flags;
        vk_command_buffer_begin_info.pInheritanceInfo = command_buffer_inheritance_info_ptr;
        VkResult result =
            _vkBeginCommandBuffer(((VulkanCommandBuffer*)command_buffer)->getResource(), &vk_command_buffer_begin_info);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("_vkBeginCommandBuffer failed!");
            return false;
        }
    }

    bool VulkanRHI::endCommandBufferPFN(RHICommandBuffer* command_buffer
    )
    {
        VkResult result = _vkEndCommandBuffer(((VulkanCommandBuffer*)command_buffer)->getResource());

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("_vkEndCommandBuffer failed!");
            return false;
        }
    }

    void VulkanRHI::cmdBeginRenderPassPFN(RHICommandBuffer* command_buffer,
	    const RHIRenderPassBeginInfo* p_render_pass_begin,
	    RHISubpassContents contents
    )
    {
        VkOffset2D offset_2d {};
        offset_2d.x = p_render_pass_begin->renderArea.offset.x;
        offset_2d.y = p_render_pass_begin->renderArea.offset.y;

        VkExtent2D extent_2d {};
        extent_2d.width  = p_render_pass_begin->renderArea.extent.width;
        extent_2d.height = p_render_pass_begin->renderArea.extent.height;

        VkRect2D rect_2d {};
        rect_2d.offset = offset_2d;
        rect_2d.extent = extent_2d;

        // clear_values
        int                       clear_value_size = p_render_pass_begin->clearValueCount;
        std::vector<VkClearValue> vk_clear_value_list(clear_value_size);
        for (int i = 0; i < clear_value_size; ++i)
        {
            const auto& rhi_clear_value_element = p_render_pass_begin->pClearValues[i];
            auto&       vk_clear_value_element  = vk_clear_value_list[i];

            VkClearColorValue vk_clear_color_value;
            vk_clear_color_value.float32[0] = rhi_clear_value_element.color.float32[0];
            vk_clear_color_value.float32[1] = rhi_clear_value_element.color.float32[1];
            vk_clear_color_value.float32[2] = rhi_clear_value_element.color.float32[2];
            vk_clear_color_value.float32[3] = rhi_clear_value_element.color.float32[3];
            vk_clear_color_value.int32[0]   = rhi_clear_value_element.color.int32[0];
            vk_clear_color_value.int32[1]   = rhi_clear_value_element.color.int32[1];
            vk_clear_color_value.int32[2]   = rhi_clear_value_element.color.int32[2];
            vk_clear_color_value.int32[3]   = rhi_clear_value_element.color.int32[3];
            vk_clear_color_value.uint32[0]  = rhi_clear_value_element.color.uint32[0];
            vk_clear_color_value.uint32[1]  = rhi_clear_value_element.color.uint32[1];
            vk_clear_color_value.uint32[2]  = rhi_clear_value_element.color.uint32[2];
            vk_clear_color_value.uint32[3]  = rhi_clear_value_element.color.uint32[3];

            VkClearDepthStencilValue vk_clear_depth_stencil_value;
            vk_clear_depth_stencil_value.depth   = rhi_clear_value_element.depthStencil.depth;
            vk_clear_depth_stencil_value.stencil = rhi_clear_value_element.depthStencil.stencil;

            vk_clear_value_element.color        = vk_clear_color_value;
            vk_clear_value_element.depthStencil = vk_clear_depth_stencil_value;
        };

        VkRenderPassBeginInfo vk_render_pass_begin_info {};
        vk_render_pass_begin_info.sType           = (VkStructureType)p_render_pass_begin->sType;
        vk_render_pass_begin_info.pNext           = p_render_pass_begin->pNext;
        vk_render_pass_begin_info.renderPass      = ((VulkanRenderPass*)p_render_pass_begin->renderPass)->getResource();
        vk_render_pass_begin_info.framebuffer = ((VulkanFramebuffer*)p_render_pass_begin->framebuffer)->getResource();
        vk_render_pass_begin_info.renderArea      = rect_2d;
        vk_render_pass_begin_info.clearValueCount = p_render_pass_begin->clearValueCount;
        vk_render_pass_begin_info.pClearValues    = vk_clear_value_list.data();

        return _vkCmdBeginRenderPass(((VulkanCommandBuffer*)command_buffer)->getResource(),
                                     &vk_render_pass_begin_info,
                                     (VkSubpassContents)contents);
    }

    void VulkanRHI::cmdNextSubpassPFN(RHICommandBuffer* command_buffer,
	    RHISubpassContents contents
    )
    {
        return _vkCmdNextSubpass(((VulkanCommandBuffer*)command_buffer)->getResource(), ((VkSubpassContents)contents));
    }

    void VulkanRHI::cmdEndRenderPassPFN(RHICommandBuffer* command_buffer
    )
    {
        return _vkCmdEndRenderPass(((VulkanCommandBuffer*)command_buffer)->getResource());
    }

    void VulkanRHI::cmdBindPipelinePFN(RHICommandBuffer* command_buffer,
	    RHIPipelineBindPoint pipeline_bind_point,
	    RHIPipeline* pipeline
    )
    {
        return _vkCmdBindPipeline(((VulkanCommandBuffer*)command_buffer)->getResource(),
                                  (VkPipelineBindPoint)pipeline_bind_point,
                                  ((VulkanPipeline*)pipeline)->getResource());
    }

    void VulkanRHI::cmdSetViewportPFN(RHICommandBuffer* command_buffer,
	    uint32_t first_viewport,
	    uint32_t viewport_count,
	    const RHIViewport* p_viewports
    )
    {
        // viewport
        int                     viewport_size = viewport_count;
        std::vector<VkViewport> vk_viewport_list(viewport_size);
        for (int i = 0; i < viewport_size; ++i)
        {
            const auto& rhi_viewport_element = p_viewports[i];
            auto&       vk_viewport_element  = vk_viewport_list[i];

            vk_viewport_element.x        = rhi_viewport_element.x;
            vk_viewport_element.y        = rhi_viewport_element.y;
            vk_viewport_element.width    = rhi_viewport_element.width;
            vk_viewport_element.height   = rhi_viewport_element.height;
            vk_viewport_element.minDepth = rhi_viewport_element.minDepth;
            vk_viewport_element.maxDepth = rhi_viewport_element.maxDepth;
        };

        return _vkCmdSetViewport(((VulkanCommandBuffer*)command_buffer)->getResource(),
                                 first_viewport,
                                 viewport_count,
                                 vk_viewport_list.data());
    }

    void VulkanRHI::cmdSetScissorPFN(RHICommandBuffer* command_buffer,
	    uint32_t first_scissor,
	    uint32_t scissor_count,
	    const RHIRect2D* p_scissors
    )
    {
        // rect_2d
        int                   rect_2d_size = scissor_count;
        std::vector<VkRect2D> vk_rect_2d_list(rect_2d_size);
        for (int i = 0; i < rect_2d_size; ++i)
        {
            const auto& rhi_rect_2d_element = p_scissors[i];
            auto&       vk_rect_2d_element  = vk_rect_2d_list[i];

            VkOffset2D offset_2d {};
            offset_2d.x = rhi_rect_2d_element.offset.x;
            offset_2d.y = rhi_rect_2d_element.offset.y;

            VkExtent2D extent_2d {};
            extent_2d.width  = rhi_rect_2d_element.extent.width;
            extent_2d.height = rhi_rect_2d_element.extent.height;

            vk_rect_2d_element.offset = (VkOffset2D)offset_2d;
            vk_rect_2d_element.extent = (VkExtent2D)extent_2d;
        };

        return _vkCmdSetScissor(((VulkanCommandBuffer*)command_buffer)->getResource(),
                                first_scissor,
                                scissor_count,
                                vk_rect_2d_list.data());
    }

    void VulkanRHI::cmdBindIndexBufferPFN(RHICommandBuffer* command_buffer,
	    RHIBuffer* buffer,
	    RHIDeviceSize offset,
	    RHIIndexType index_type
    )
    {
        return _vkCmdBindIndexBuffer(((VulkanCommandBuffer*)command_buffer)->getResource(),
                                     ((VulkanBuffer*)buffer)->getResource(),
                                     (VkDeviceSize)offset,
                                     (VkIndexType)index_type);
    }

    void VulkanRHI::cmdDrawIndexedPFN(RHICommandBuffer* command_buffer,
                                      uint32_t          index_count,
                                      uint32_t          instance_count,
                                      uint32_t          first_index,
                                      int32_t           vertex_offset,
                                      uint32_t          first_instance)
    {
        return _vkCmdDrawIndexed(((VulkanCommandBuffer*)command_buffer)->getResource(),
                                  index_count,

                                 instance_count,
                                 first_index,
                                 vertex_offset,
                                 first_instance);
    }

    void VulkanRHI::cmdClearAttachmentsPFN(RHICommandBuffer* command_buffer,
	    uint32_t attachment_count,
	    const RHIClearAttachment* p_attachments,
	    uint32_t rect_count,
	    const RHIClearRect* p_rects
    )
    {
        // clear_attachment
        int                            clear_attachment_size = attachment_count;
        std::vector<VkClearAttachment> vk_clear_attachment_list(clear_attachment_size);
        for (int i = 0; i < clear_attachment_size; ++i)
        {
            const auto& rhi_clear_attachment_element = p_attachments[i];
            auto&       vk_clear_attachment_element  = vk_clear_attachment_list[i];

            VkClearColorValue vk_clear_color_value;
            vk_clear_color_value.float32[0] = rhi_clear_attachment_element.clearValue.color.float32[0];
            vk_clear_color_value.float32[1] = rhi_clear_attachment_element.clearValue.color.float32[1];
            vk_clear_color_value.float32[2] = rhi_clear_attachment_element.clearValue.color.float32[2];
            vk_clear_color_value.float32[3] = rhi_clear_attachment_element.clearValue.color.float32[3];
            vk_clear_color_value.int32[0]   = rhi_clear_attachment_element.clearValue.color.int32[0];
            vk_clear_color_value.int32[1]   = rhi_clear_attachment_element.clearValue.color.int32[1];
            vk_clear_color_value.int32[2]   = rhi_clear_attachment_element.clearValue.color.int32[2];
            vk_clear_color_value.int32[3]   = rhi_clear_attachment_element.clearValue.color.int32[3];
            vk_clear_color_value.uint32[0]  = rhi_clear_attachment_element.clearValue.color.uint32[0];
            vk_clear_color_value.uint32[1]  = rhi_clear_attachment_element.clearValue.color.uint32[1];
            vk_clear_color_value.uint32[2]  = rhi_clear_attachment_element.clearValue.color.uint32[2];
            vk_clear_color_value.uint32[3]  = rhi_clear_attachment_element.clearValue.color.uint32[3];

            VkClearDepthStencilValue vk_clear_depth_stencil_value;
            vk_clear_depth_stencil_value.depth   = rhi_clear_attachment_element.clearValue.depthStencil.depth;
            vk_clear_depth_stencil_value.stencil = rhi_clear_attachment_element.clearValue.depthStencil.stencil;

            vk_clear_attachment_element.clearValue.color        = vk_clear_color_value;
            vk_clear_attachment_element.clearValue.depthStencil = vk_clear_depth_stencil_value;
            vk_clear_attachment_element.aspectMask              = rhi_clear_attachment_element.aspectMask;
            vk_clear_attachment_element.colorAttachment         = rhi_clear_attachment_element.colorAttachment;
        };

        // clear_rect
        int                      clear_rect_size = rect_count;
        std::vector<VkClearRect> vk_clear_rect_list(clear_rect_size);
        for (int i = 0; i < clear_rect_size; ++i)
        {
            const auto& rhi_clear_rect_element = p_rects[i];
            auto&       vk_clear_rect_element  = vk_clear_rect_list[i];

            VkOffset2D offset_2d {};
            offset_2d.x = rhi_clear_rect_element.rect.offset.x;
            offset_2d.y = rhi_clear_rect_element.rect.offset.y;

            VkExtent2D extent_2d {};
            extent_2d.width  = rhi_clear_rect_element.rect.extent.width;
            extent_2d.height = rhi_clear_rect_element.rect.extent.height;

            vk_clear_rect_element.rect.offset    = (VkOffset2D)offset_2d;
            vk_clear_rect_element.rect.extent    = (VkExtent2D)extent_2d;
            vk_clear_rect_element.baseArrayLayer = rhi_clear_rect_element.baseArrayLayer;
            vk_clear_rect_element.layerCount     = rhi_clear_rect_element.layerCount;
        };

        return _vkCmdClearAttachments(((VulkanCommandBuffer*)command_buffer)->getResource(),
                                      attachment_count,
                                      vk_clear_attachment_list.data(),
                                      rect_count,
                                      vk_clear_rect_list.data());
    }

    bool VulkanRHI::beginCommandBuffer(RHICommandBuffer* command_buffer,
	    const RHICommandBufferBeginInfo* p_begin_info
    )
    {
        VkCommandBufferInheritanceInfo        command_buffer_inheritance_info {};
        const VkCommandBufferInheritanceInfo* command_buffer_inheritance_info_ptr = nullptr;
        if (p_begin_info->pInheritanceInfo != nullptr)
        {
            command_buffer_inheritance_info.sType = (VkStructureType)(p_begin_info->pInheritanceInfo->sType);
            command_buffer_inheritance_info.pNext = (const void*)p_begin_info->pInheritanceInfo->pNext;
            command_buffer_inheritance_info.renderPass =
                ((VulkanRenderPass*)p_begin_info->pInheritanceInfo->renderPass)->getResource();
            command_buffer_inheritance_info.subpass = p_begin_info->pInheritanceInfo->subpass;
            command_buffer_inheritance_info.framebuffer =
                ((VulkanFramebuffer*)(p_begin_info->pInheritanceInfo->framebuffer))->getResource();
            command_buffer_inheritance_info.occlusionQueryEnable =
                (VkBool32)p_begin_info->pInheritanceInfo->occlusionQueryEnable;
            command_buffer_inheritance_info.queryFlags = (VkQueryControlFlags)p_begin_info->pInheritanceInfo->queryFlags;
            command_buffer_inheritance_info.pipelineStatistics =
                (VkQueryPipelineStatisticFlags)p_begin_info->pInheritanceInfo->pipelineStatistics;

            command_buffer_inheritance_info_ptr = &command_buffer_inheritance_info;
        }

        VkCommandBufferBeginInfo command_buffer_begin_info {};
        command_buffer_begin_info.sType            = (VkStructureType)p_begin_info->sType;
        command_buffer_begin_info.pNext            = (const void*)p_begin_info->pNext;
        command_buffer_begin_info.flags            = (VkCommandBufferUsageFlags)p_begin_info->flags;
        command_buffer_begin_info.pInheritanceInfo = command_buffer_inheritance_info_ptr;

        VkResult result =
            vkBeginCommandBuffer(((VulkanCommandBuffer*)command_buffer)->getResource(), &command_buffer_begin_info);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("vkBeginCommandBuffer failed!");
            return false;
        }
    }

    void VulkanRHI::cmdCopyImageToBuffer(RHICommandBuffer* command_buffer,
	    RHIImage* src_image,
	    RHIImageLayout src_image_layout,
	    RHIBuffer* dst_buffer,
	    uint32_t region_count,
	    const RHIBufferImageCopy* p_regions
    )
    {
        // buffer_image_copy
        int                            buffer_image_copy_size = region_count;
        std::vector<VkBufferImageCopy> vk_buffer_image_copy_list(buffer_image_copy_size);
        for (int i = 0; i < buffer_image_copy_size; ++i)
        {
            const auto& rhi_buffer_image_copy_element = p_regions[i];
            auto&       vk_buffer_image_copy_element  = vk_buffer_image_copy_list[i];

            VkImageSubresourceLayers image_subresource_layers {};
            image_subresource_layers.aspectMask =
                (VkImageAspectFlags)rhi_buffer_image_copy_element.imageSubresource.aspectMask;
            image_subresource_layers.mipLevel       = rhi_buffer_image_copy_element.imageSubresource.mipLevel;
            image_subresource_layers.baseArrayLayer = rhi_buffer_image_copy_element.imageSubresource.baseArrayLayer;
            image_subresource_layers.layerCount     = rhi_buffer_image_copy_element.imageSubresource.layerCount;

            VkOffset3D offset_3d {};
            offset_3d.x = rhi_buffer_image_copy_element.imageOffset.x;
            offset_3d.y = rhi_buffer_image_copy_element.imageOffset.y;
            offset_3d.z = rhi_buffer_image_copy_element.imageOffset.z;

            VkExtent3D extent_3d {};
            extent_3d.width  = rhi_buffer_image_copy_element.imageExtent.width;
            extent_3d.height = rhi_buffer_image_copy_element.imageExtent.height;
            extent_3d.depth  = rhi_buffer_image_copy_element.imageExtent.depth;

            VkBufferImageCopy buffer_image_copy {};
            buffer_image_copy.bufferOffset      = (VkDeviceSize)rhi_buffer_image_copy_element.bufferOffset;
            buffer_image_copy.bufferRowLength   = rhi_buffer_image_copy_element.bufferRowLength;
            buffer_image_copy.bufferImageHeight = rhi_buffer_image_copy_element.bufferImageHeight;
            buffer_image_copy.imageSubresource  = image_subresource_layers;
            buffer_image_copy.imageOffset       = offset_3d;
            buffer_image_copy.imageExtent       = extent_3d;

            vk_buffer_image_copy_element.bufferOffset      = (VkDeviceSize)rhi_buffer_image_copy_element.bufferOffset;
            vk_buffer_image_copy_element.bufferRowLength   = rhi_buffer_image_copy_element.bufferRowLength;
            vk_buffer_image_copy_element.bufferImageHeight = rhi_buffer_image_copy_element.bufferImageHeight;
            vk_buffer_image_copy_element.imageSubresource  = image_subresource_layers;
            vk_buffer_image_copy_element.imageOffset       = offset_3d;
            vk_buffer_image_copy_element.imageExtent       = extent_3d;
        };

        vkCmdCopyImageToBuffer(((VulkanCommandBuffer*)command_buffer)->getResource(),
                               ((VulkanImage*)src_image)->getResource(),
                               (VkImageLayout)src_image_layout,
                               ((VulkanBuffer*)dst_buffer)->getResource(),
                               region_count,
                               vk_buffer_image_copy_list.data());
    }

    void VulkanRHI::cmdCopyImageToImage(RHICommandBuffer* command_buffer,
	    RHIImage* src_image,
	    RHIImageAspectFlagBits src_flag,
	    RHIImage* dst_image,
	    RHIImageAspectFlagBits dst_flag,
	    uint32_t width,
	    uint32_t height
    )
    {
        VkImageCopy imagecopy_region    = {};
        imagecopy_region.srcSubresource = {(VkImageAspectFlags)src_flag, 0, 0, 1};
        imagecopy_region.srcOffset      = {0, 0, 0};
        imagecopy_region.dstSubresource = {(VkImageAspectFlags)dst_flag, 0, 0, 1};
        imagecopy_region.dstOffset      = {0, 0, 0};
        imagecopy_region.extent         = {width, height, 1};

        vkCmdCopyImage(((VulkanCommandBuffer*)command_buffer)->getResource(),
                       ((VulkanImage*)src_image)->getResource(),
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       ((VulkanImage*)dst_image)->getResource(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1,
                       &imagecopy_region);
    }

    void VulkanRHI::cmdCopyBuffer(RHICommandBuffer* command_buffer,
	    RHIBuffer* src_buffer,
	    RHIBuffer* dst_buffer,
	    uint32_t region_count,
	    RHIBufferCopy* p_regions
    )
    {
        VkBufferCopy copy_region {};
        copy_region.srcOffset = p_regions->srcOffset;
        copy_region.dstOffset = p_regions->dstOffset;
        copy_region.size      = p_regions->size;

        vkCmdCopyBuffer(((VulkanCommandBuffer*)command_buffer)->getResource(),
                        ((VulkanBuffer*)src_buffer)->getResource(),
                        ((VulkanBuffer*)dst_buffer)->getResource(),
                        region_count,
                        &copy_region);
    }

    void VulkanRHI::cmdDraw(RHICommandBuffer* command_buffer,
	    uint32_t vertex_count,
	    uint32_t instance_count,
	    uint32_t first_vertex,
	    uint32_t first_instance
    )
    {
        vkCmdDraw(((VulkanCommandBuffer*)command_buffer)->getResource(),
                  vertex_count,
                  instance_count,
                  first_vertex,
                  first_instance);
    }

    void VulkanRHI::cmdDispatch(RHICommandBuffer* command_buffer,
	    uint32_t group_count_x,
	    uint32_t group_count_y,
	    uint32_t group_count_z
    )
    {
        vkCmdDispatch(
            ((VulkanCommandBuffer*)command_buffer)->getResource(), group_count_x, group_count_y, group_count_z);
    }

    void VulkanRHI::cmdDispatchIndirect(RHICommandBuffer* command_buffer,
	    RHIBuffer* buffer,
	    RHIDeviceSize offset
    )
    {
        vkCmdDispatchIndirect(
            ((VulkanCommandBuffer*)command_buffer)->getResource(), ((VulkanBuffer*)buffer)->getResource(), offset);
    }

    void VulkanRHI::cmdPipelineBarrier(RHICommandBuffer* command_buffer,
	    RHIPipelineStageFlags src_stage_mask,
	    RHIPipelineStageFlags dst_stage_mask,
	    RHIDependencyFlags dependency_flags,
	    uint32_t memory_barrier_count,
	    const RHIMemoryBarrier* p_memory_barriers,
	    uint32_t buffer_memory_barrier_count,
	    const RHIBufferMemoryBarrier* p_buffer_memory_barriers,
	    uint32_t image_memory_barrier_count,
	    const RHIImageMemoryBarrier* p_image_memory_barriers
    )
    {

        // memory_barrier
        int                          memory_barrier_size = memory_barrier_count;
        std::vector<VkMemoryBarrier> vk_memory_barrier_list(memory_barrier_size);
        for (int i = 0; i < memory_barrier_size; ++i)
        {
            const auto& rhi_memory_barrier_element = p_memory_barriers[i];
            auto&       vk_memory_barrier_element  = vk_memory_barrier_list[i];

            vk_memory_barrier_element.sType         = (VkStructureType)rhi_memory_barrier_element.sType;
            vk_memory_barrier_element.pNext         = (const void*)rhi_memory_barrier_element.pNext;
            vk_memory_barrier_element.srcAccessMask = (VkAccessFlags)rhi_memory_barrier_element.srcAccessMask;
            vk_memory_barrier_element.dstAccessMask = (VkAccessFlags)rhi_memory_barrier_element.dstAccessMask;
        };

        // buffer_memory_barrier
        int                                buffer_memory_barrier_size = buffer_memory_barrier_count;
        std::vector<VkBufferMemoryBarrier> vk_buffer_memory_barrier_list(buffer_memory_barrier_size);
        for (int i = 0; i < buffer_memory_barrier_size; ++i)
        {
            const auto& rhi_buffer_memory_barrier_element = p_buffer_memory_barriers[i];
            auto&       vk_buffer_memory_barrier_element  = vk_buffer_memory_barrier_list[i];

            vk_buffer_memory_barrier_element.sType = (VkStructureType)rhi_buffer_memory_barrier_element.sType;
            vk_buffer_memory_barrier_element.pNext = (const void*)rhi_buffer_memory_barrier_element.pNext;
            vk_buffer_memory_barrier_element.srcAccessMask =
                (VkAccessFlags)rhi_buffer_memory_barrier_element.srcAccessMask;
            vk_buffer_memory_barrier_element.dstAccessMask =
                (VkAccessFlags)rhi_buffer_memory_barrier_element.dstAccessMask;
            vk_buffer_memory_barrier_element.srcQueueFamilyIndex =
                rhi_buffer_memory_barrier_element.srcQueueFamilyIndex;
            vk_buffer_memory_barrier_element.dstQueueFamilyIndex =
                rhi_buffer_memory_barrier_element.dstQueueFamilyIndex;
            vk_buffer_memory_barrier_element.buffer =
                ((VulkanBuffer*)rhi_buffer_memory_barrier_element.buffer)->getResource();
            vk_buffer_memory_barrier_element.offset = (VkDeviceSize)rhi_buffer_memory_barrier_element.offset;
            vk_buffer_memory_barrier_element.size   = (VkDeviceSize)rhi_buffer_memory_barrier_element.size;
        };

        // image_memory_barrier
        int                               image_memory_barrier_size = image_memory_barrier_count;
        std::vector<VkImageMemoryBarrier> vk_image_memory_barrier_list(image_memory_barrier_size);
        for (int i = 0; i < image_memory_barrier_size; ++i)
        {
            const auto& rhi_image_memory_barrier_element = p_image_memory_barriers[i];
            auto&       vk_image_memory_barrier_element  = vk_image_memory_barrier_list[i];

            VkImageSubresourceRange image_subresource_range {};
            image_subresource_range.aspectMask =
                (VkImageAspectFlags)rhi_image_memory_barrier_element.subresourceRange.aspectMask;
            image_subresource_range.baseMipLevel   = rhi_image_memory_barrier_element.subresourceRange.baseMipLevel;
            image_subresource_range.levelCount     = rhi_image_memory_barrier_element.subresourceRange.levelCount;
            image_subresource_range.baseArrayLayer = rhi_image_memory_barrier_element.subresourceRange.baseArrayLayer;
            image_subresource_range.layerCount     = rhi_image_memory_barrier_element.subresourceRange.layerCount;

            vk_image_memory_barrier_element.sType = (VkStructureType)rhi_image_memory_barrier_element.sType;
            vk_image_memory_barrier_element.pNext = (const void*)rhi_image_memory_barrier_element.pNext;
            vk_image_memory_barrier_element.srcAccessMask =
                (VkAccessFlags)rhi_image_memory_barrier_element.srcAccessMask;
            vk_image_memory_barrier_element.dstAccessMask =
                (VkAccessFlags)rhi_image_memory_barrier_element.dstAccessMask;
            vk_image_memory_barrier_element.oldLayout = (VkImageLayout)rhi_image_memory_barrier_element.oldLayout;
            vk_image_memory_barrier_element.newLayout = (VkImageLayout)rhi_image_memory_barrier_element.newLayout;
            vk_image_memory_barrier_element.srcQueueFamilyIndex = rhi_image_memory_barrier_element.srcQueueFamilyIndex;
            vk_image_memory_barrier_element.dstQueueFamilyIndex = rhi_image_memory_barrier_element.dstQueueFamilyIndex;
            vk_image_memory_barrier_element.image =
                ((VulkanImage*)rhi_image_memory_barrier_element.image)->getResource();
            vk_image_memory_barrier_element.subresourceRange = image_subresource_range;
        };

        vkCmdPipelineBarrier(((VulkanCommandBuffer*)command_buffer)->getResource(),
                             (RHIPipelineStageFlags)src_stage_mask,
                             (RHIPipelineStageFlags)dst_stage_mask,
                             (RHIDependencyFlags)dependency_flags,
                             memory_barrier_count,
                             vk_memory_barrier_list.data(),
                             buffer_memory_barrier_count,
                             vk_buffer_memory_barrier_list.data(),
                             image_memory_barrier_count,
                             vk_image_memory_barrier_list.data());
    }

    bool VulkanRHI::endCommandBuffer(RHICommandBuffer* command_buffer
    )
    {
        VkResult result = vkEndCommandBuffer(((VulkanCommandBuffer*)command_buffer)->getResource());

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("vkEndCommandBuffer failed!");
            return false;
        }
    }

    void VulkanRHI::updateDescriptorSets(uint32_t descriptor_write_count,
	    const RHIWriteDescriptorSet* p_descriptor_writes,
	    uint32_t descriptor_copy_count,
	    const RHICopyDescriptorSet* p_descriptor_copies
    )
    {
        // write_descriptor_set
        int                               write_descriptor_set_size = descriptor_write_count;
        std::vector<VkWriteDescriptorSet> vk_write_descriptor_set_list(write_descriptor_set_size);
        int                               image_info_count  = 0;
        int                               buffer_info_count = 0;
        for (int i = 0; i < write_descriptor_set_size; ++i)
        {
            const auto& rhi_write_descriptor_set_element = p_descriptor_writes[i];
            if (rhi_write_descriptor_set_element.pImageInfo != nullptr)
            {
                image_info_count++;
            }
            if (rhi_write_descriptor_set_element.pBufferInfo != nullptr)
            {
                buffer_info_count++;
            }
        }
        std::vector<VkDescriptorImageInfo>  vk_descriptor_image_info_list(image_info_count);
        std::vector<VkDescriptorBufferInfo> vk_descriptor_buffer_info_list(buffer_info_count);
        int                                 image_info_current  = 0;
        int                                 buffer_info_current = 0;

        for (int i = 0; i < write_descriptor_set_size; ++i)
        {
            const auto& rhi_write_descriptor_set_element = p_descriptor_writes[i];
            auto&       vk_write_descriptor_set_element  = vk_write_descriptor_set_list[i];

            const VkDescriptorImageInfo* vk_descriptor_image_info_ptr = nullptr;
            if (rhi_write_descriptor_set_element.pImageInfo != nullptr)
            {
                auto& vk_descriptor_image_info = vk_descriptor_image_info_list[image_info_current];
                if (rhi_write_descriptor_set_element.pImageInfo->sampler == nullptr)
                {
                    vk_descriptor_image_info.sampler = nullptr;
                }
                else
                {
                    vk_descriptor_image_info.sampler =
                        ((VulkanSampler*)rhi_write_descriptor_set_element.pImageInfo->sampler)->getResource();
                }
                vk_descriptor_image_info.imageView =
                    ((VulkanImageView*)rhi_write_descriptor_set_element.pImageInfo->imageView)->getResource();
                vk_descriptor_image_info.imageLayout =
                    (VkImageLayout)rhi_write_descriptor_set_element.pImageInfo->imageLayout;

                vk_descriptor_image_info_ptr = &vk_descriptor_image_info;
                image_info_current++;
            }

            const VkDescriptorBufferInfo* vk_descriptor_buffer_info_ptr = nullptr;
            if (rhi_write_descriptor_set_element.pBufferInfo != nullptr)
            {
                auto& vk_descriptor_buffer_info = vk_descriptor_buffer_info_list[buffer_info_current];
                vk_descriptor_buffer_info.buffer =
                    ((VulkanBuffer*)rhi_write_descriptor_set_element.pBufferInfo->buffer)->getResource();
                vk_descriptor_buffer_info.offset = (VkDeviceSize)rhi_write_descriptor_set_element.pBufferInfo->offset;
                vk_descriptor_buffer_info.range  = (VkDeviceSize)rhi_write_descriptor_set_element.pBufferInfo->range;

                vk_descriptor_buffer_info_ptr = &vk_descriptor_buffer_info;
                buffer_info_current++;
            }

            vk_write_descriptor_set_element.sType = (VkStructureType)rhi_write_descriptor_set_element.sType;
            vk_write_descriptor_set_element.pNext = (const void*)rhi_write_descriptor_set_element.pNext;
            vk_write_descriptor_set_element.dstSet =
                ((VulkanDescriptorSet*)rhi_write_descriptor_set_element.dstSet)->getResource();
            vk_write_descriptor_set_element.dstBinding      = rhi_write_descriptor_set_element.dstBinding;
            vk_write_descriptor_set_element.dstArrayElement = rhi_write_descriptor_set_element.dstArrayElement;
            vk_write_descriptor_set_element.descriptorCount = rhi_write_descriptor_set_element.descriptorCount;
            vk_write_descriptor_set_element.descriptorType =
                (VkDescriptorType)rhi_write_descriptor_set_element.descriptorType;
            vk_write_descriptor_set_element.pImageInfo  = vk_descriptor_image_info_ptr;
            vk_write_descriptor_set_element.pBufferInfo = vk_descriptor_buffer_info_ptr;
            // vk_write_descriptor_set_element.pTexelBufferView =
            // &((VulkanBufferView*)rhi_write_descriptor_set_element.pTexelBufferView)->getResource();
        };

        if (image_info_current != image_info_count || buffer_info_current != buffer_info_count)
        {
            LOG_ERROR("image_info_current != image_info_count || buffer_info_current != buffer_info_count");
            return;
        }

        // copy_descriptor_set
        int                              copy_descriptor_set_size = descriptor_copy_count;
        std::vector<VkCopyDescriptorSet> vk_copy_descriptor_set_list(copy_descriptor_set_size);
        for (int i = 0; i < copy_descriptor_set_size; ++i)
        {
            const auto&               rhi_copy_descriptor_set_element = p_descriptor_copies[i];
            
            auto&       vk_copy_descriptor_set_element  = vk_copy_descriptor_set_list[i];

            vk_copy_descriptor_set_element.sType = (VkStructureType)rhi_copy_descriptor_set_element.sType;
            vk_copy_descriptor_set_element.pNext = (const void*)rhi_copy_descriptor_set_element.pNext;
            vk_copy_descriptor_set_element.srcSet =
                ((VulkanDescriptorSet*)rhi_copy_descriptor_set_element.srcSet)->getResource();
            vk_copy_descriptor_set_element.srcBinding      = rhi_copy_descriptor_set_element.srcBinding;
            vk_copy_descriptor_set_element.srcArrayElement = rhi_copy_descriptor_set_element.srcArrayElement;
            vk_copy_descriptor_set_element.dstSet =
                ((VulkanDescriptorSet*)rhi_copy_descriptor_set_element.dstSet)->getResource();
            vk_copy_descriptor_set_element.dstBinding      = rhi_copy_descriptor_set_element.dstBinding;
            vk_copy_descriptor_set_element.dstArrayElement = rhi_copy_descriptor_set_element.dstArrayElement;
            vk_copy_descriptor_set_element.descriptorCount = rhi_copy_descriptor_set_element.descriptorCount;
        };

        vkUpdateDescriptorSets(m_device,
                               descriptor_write_count,
                               vk_write_descriptor_set_list.data(),
                               descriptor_copy_count,
                               vk_copy_descriptor_set_list.data());
    }

    bool VulkanRHI::queueSubmit(RHIQueue* queue,
	    uint32_t submit_count,
	    const RHISubmitInfo* p_submits,
	    RHIFence* fence
    ) {
        return true;
    }

    bool VulkanRHI::queueWaitIdle(RHIQueue* queue
    )
    {
        VkResult result = vkQueueWaitIdle(((VulkanQueue*)queue)->getResource());

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("vkQueueWaitIdle failed!");
            return false;
        }
    }

    void VulkanRHI::resetCommandPool()
    {
        VkResult res_reset_command_pool = _vkResetCommandPool(m_device, m_command_pools[m_current_frame_index], 0);
        if (VK_SUCCESS != res_reset_command_pool)
        {
            LOG_ERROR("failed to synchronize");
        }
    }
    void VulkanRHI::waitForFences()
    {
        VkResult res_wait_for_fences =
            _vkWaitForFences(m_device, 1, &m_is_frame_in_flight_fences[m_current_frame_index], VK_TRUE, UINT64_MAX);
        if (VK_SUCCESS != res_wait_for_fences)
        {
            LOG_ERROR("failed to synchronize!");
        }
    }

    bool VulkanRHI::waitForFences(uint32_t fence_count,
	    const RHIFence* const* p_fences,
	    RHIBool32 wait_all,
	    uint64_t timeout
    )
    {
        // fence
        int                  fence_size = fence_count;
        std::vector<VkFence> vk_fence_list(fence_size);
        for (int i = 0; i < fence_size; ++i)
        {
            const auto& rhi_fence_element = p_fences[i];
            auto&       vk_fence_element  = vk_fence_list[i];

            vk_fence_element = ((VulkanFence*)rhi_fence_element)->getResource();
        };

        VkResult result = vkWaitForFences(m_device, fence_count, vk_fence_list.data(), wait_all, timeout);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("waitForFences failed");
            return false;
        }
    }

    void VulkanRHI::getPhysicalDeviceProperties(RHIPhysicalDeviceProperties* pProperties
    )
    {
        VkPhysicalDeviceProperties vk_physical_device_properties;
        vkGetPhysicalDeviceProperties(m_physical_device, &vk_physical_device_properties);

        pProperties->apiVersion    = vk_physical_device_properties.apiVersion;
        pProperties->driverVersion = vk_physical_device_properties.driverVersion;
        pProperties->vendorID      = vk_physical_device_properties.vendorID;
        pProperties->deviceID      = vk_physical_device_properties.deviceID;
        pProperties->deviceType    = (RHIPhysicalDeviceType)vk_physical_device_properties.deviceType;
        for (uint32_t i = 0; i < RHI_MAX_PHYSICAL_DEVICE_NAME_SIZE; i++)
        {
            pProperties->deviceName[i] = vk_physical_device_properties.deviceName[i];
        }
        for (uint32_t i = 0; i < RHI_UUID_SIZE; i++)
        {
            pProperties->pipelineCacheUUID[i] = vk_physical_device_properties.pipelineCacheUUID[i];
        }
        pProperties->sparseProperties.residencyStandard2DBlockShape =
            (VkBool32)vk_physical_device_properties.sparseProperties.residencyStandard2DBlockShape;
        pProperties->sparseProperties.residencyStandard2DMultisampleBlockShape =
            (VkBool32)vk_physical_device_properties.sparseProperties.residencyStandard2DMultisampleBlockShape;
        pProperties->sparseProperties.residencyStandard3DBlockShape =
            (VkBool32)vk_physical_device_properties.sparseProperties.residencyStandard3DBlockShape;
        pProperties->sparseProperties.residencyAlignedMipSize =
            (VkBool32)vk_physical_device_properties.sparseProperties.residencyAlignedMipSize;
        pProperties->sparseProperties.residencyNonResidentStrict =
            (VkBool32)vk_physical_device_properties.sparseProperties.residencyNonResidentStrict;

        pProperties->limits.maxImageDimension1D       = vk_physical_device_properties.limits.maxImageDimension1D;
        pProperties->limits.maxImageDimension2D       = vk_physical_device_properties.limits.maxImageDimension2D;
        pProperties->limits.maxImageDimension3D       = vk_physical_device_properties.limits.maxImageDimension3D;
        pProperties->limits.maxImageDimensionCube     = vk_physical_device_properties.limits.maxImageDimensionCube;
        pProperties->limits.maxImageArrayLayers       = vk_physical_device_properties.limits.maxImageArrayLayers;
        pProperties->limits.maxTexelBufferElements    = vk_physical_device_properties.limits.maxTexelBufferElements;
        pProperties->limits.maxUniformBufferRange     = vk_physical_device_properties.limits.maxUniformBufferRange;
        pProperties->limits.maxStorageBufferRange     = vk_physical_device_properties.limits.maxStorageBufferRange;
        pProperties->limits.maxPushConstantsSize      = vk_physical_device_properties.limits.maxPushConstantsSize;
        pProperties->limits.maxMemoryAllocationCount  = vk_physical_device_properties.limits.maxMemoryAllocationCount;
        pProperties->limits.maxSamplerAllocationCount = vk_physical_device_properties.limits.maxSamplerAllocationCount;
        pProperties->limits.bufferImageGranularity =
            (VkDeviceSize)vk_physical_device_properties.limits.bufferImageGranularity;
        pProperties->limits.sparseAddressSpaceSize =
            (VkDeviceSize)vk_physical_device_properties.limits.sparseAddressSpaceSize;
        pProperties->limits.maxBoundDescriptorSets = vk_physical_device_properties.limits.maxBoundDescriptorSets;
        pProperties->limits.maxPerStageDescriptorSamplers =
            vk_physical_device_properties.limits.maxPerStageDescriptorSamplers;
        pProperties->limits.maxPerStageDescriptorUniformBuffers =
            vk_physical_device_properties.limits.maxPerStageDescriptorUniformBuffers;
        pProperties->limits.maxPerStageDescriptorStorageBuffers =
            vk_physical_device_properties.limits.maxPerStageDescriptorStorageBuffers;
        pProperties->limits.maxPerStageDescriptorSampledImages =
            vk_physical_device_properties.limits.maxPerStageDescriptorSampledImages;
        pProperties->limits.maxPerStageDescriptorStorageImages =
            vk_physical_device_properties.limits.maxPerStageDescriptorStorageImages;
        pProperties->limits.maxPerStageDescriptorInputAttachments =
            vk_physical_device_properties.limits.maxPerStageDescriptorInputAttachments;
        pProperties->limits.maxPerStageResources     = vk_physical_device_properties.limits.maxPerStageResources;
        pProperties->limits.maxDescriptorSetSamplers = vk_physical_device_properties.limits.maxDescriptorSetSamplers;
        pProperties->limits.maxDescriptorSetUniformBuffers =
            vk_physical_device_properties.limits.maxDescriptorSetUniformBuffers;
        pProperties->limits.maxDescriptorSetUniformBuffersDynamic =
            vk_physical_device_properties.limits.maxDescriptorSetUniformBuffersDynamic;
        pProperties->limits.maxDescriptorSetStorageBuffers =
            vk_physical_device_properties.limits.maxDescriptorSetStorageBuffers;
        pProperties->limits.maxDescriptorSetStorageBuffersDynamic =
            vk_physical_device_properties.limits.maxDescriptorSetStorageBuffersDynamic;
        pProperties->limits.maxDescriptorSetSampledImages =
            vk_physical_device_properties.limits.maxDescriptorSetSampledImages;
        pProperties->limits.maxDescriptorSetStorageImages =
            vk_physical_device_properties.limits.maxDescriptorSetStorageImages;
        pProperties->limits.maxDescriptorSetInputAttachments =
            vk_physical_device_properties.limits.maxDescriptorSetInputAttachments;
        pProperties->limits.maxVertexInputAttributes = vk_physical_device_properties.limits.maxVertexInputAttributes;
        pProperties->limits.maxVertexInputBindings   = vk_physical_device_properties.limits.maxVertexInputBindings;
        pProperties->limits.maxVertexInputAttributeOffset =
            vk_physical_device_properties.limits.maxVertexInputAttributeOffset;
        pProperties->limits.maxVertexInputBindingStride =
            vk_physical_device_properties.limits.maxVertexInputBindingStride;
        pProperties->limits.maxVertexOutputComponents = vk_physical_device_properties.limits.maxVertexOutputComponents;
        pProperties->limits.maxTessellationGenerationLevel =
            vk_physical_device_properties.limits.maxTessellationGenerationLevel;
        pProperties->limits.maxTessellationPatchSize = vk_physical_device_properties.limits.maxTessellationPatchSize;
        pProperties->limits.maxTessellationControlPerVertexInputComponents =
            vk_physical_device_properties.limits.maxTessellationControlPerVertexInputComponents;
        pProperties->limits.maxTessellationControlPerVertexOutputComponents =
            vk_physical_device_properties.limits.maxTessellationControlPerVertexOutputComponents;
        pProperties->limits.maxTessellationControlPerPatchOutputComponents =
            vk_physical_device_properties.limits.maxTessellationControlPerPatchOutputComponents;
        pProperties->limits.maxTessellationControlTotalOutputComponents =
            vk_physical_device_properties.limits.maxTessellationControlTotalOutputComponents;
        pProperties->limits.maxTessellationEvaluationInputComponents =
            vk_physical_device_properties.limits.maxTessellationEvaluationInputComponents;
        pProperties->limits.maxTessellationEvaluationOutputComponents =
            vk_physical_device_properties.limits.maxTessellationEvaluationOutputComponents;
        pProperties->limits.maxGeometryShaderInvocations =
            vk_physical_device_properties.limits.maxGeometryShaderInvocations;
        pProperties->limits.maxGeometryInputComponents =
            vk_physical_device_properties.limits.maxGeometryInputComponents;
        pProperties->limits.maxGeometryOutputComponents =
            vk_physical_device_properties.limits.maxGeometryOutputComponents;
        pProperties->limits.maxGeometryOutputVertices = vk_physical_device_properties.limits.maxGeometryOutputVertices;
        pProperties->limits.maxGeometryTotalOutputComponents =
            vk_physical_device_properties.limits.maxGeometryTotalOutputComponents;
        pProperties->limits.maxFragmentInputComponents =
            vk_physical_device_properties.limits.maxFragmentInputComponents;
        pProperties->limits.maxFragmentOutputAttachments =
            vk_physical_device_properties.limits.maxFragmentOutputAttachments;
        pProperties->limits.maxFragmentDualSrcAttachments =
            vk_physical_device_properties.limits.maxFragmentDualSrcAttachments;
        pProperties->limits.maxFragmentCombinedOutputResources =
            vk_physical_device_properties.limits.maxFragmentCombinedOutputResources;
        pProperties->limits.maxComputeSharedMemorySize =
            vk_physical_device_properties.limits.maxComputeSharedMemorySize;
        for (uint32_t i = 0; i < 3; i++)
        {
            pProperties->limits.maxComputeWorkGroupCount[i] =
                vk_physical_device_properties.limits.maxComputeWorkGroupCount[i];
        }
        pProperties->limits.maxComputeWorkGroupInvocations =
            vk_physical_device_properties.limits.maxComputeWorkGroupInvocations;
        for (uint32_t i = 0; i < 3; i++)
        {
            pProperties->limits.maxComputeWorkGroupSize[i] =
                vk_physical_device_properties.limits.maxComputeWorkGroupSize[i];
        }
        pProperties->limits.subPixelPrecisionBits    = vk_physical_device_properties.limits.subPixelPrecisionBits;
        pProperties->limits.subTexelPrecisionBits    = vk_physical_device_properties.limits.subTexelPrecisionBits;
        pProperties->limits.mipmapPrecisionBits      = vk_physical_device_properties.limits.mipmapPrecisionBits;
        pProperties->limits.maxDrawIndexedIndexValue = vk_physical_device_properties.limits.maxDrawIndexedIndexValue;
        pProperties->limits.maxDrawIndirectCount     = vk_physical_device_properties.limits.maxDrawIndirectCount;
        pProperties->limits.maxSamplerLodBias        = vk_physical_device_properties.limits.maxSamplerLodBias;
        pProperties->limits.maxSamplerAnisotropy     = vk_physical_device_properties.limits.maxSamplerAnisotropy;
        pProperties->limits.maxViewports             = vk_physical_device_properties.limits.maxViewports;
        for (uint32_t i = 0; i < 2; i++)
        {
            pProperties->limits.maxViewportDimensions[i] =
                vk_physical_device_properties.limits.maxViewportDimensions[i];
        }
        for (uint32_t i = 0; i < 2; i++)
        {
            pProperties->limits.viewportBoundsRange[i] = vk_physical_device_properties.limits.viewportBoundsRange[i];
        }
        pProperties->limits.viewportSubPixelBits  = vk_physical_device_properties.limits.viewportSubPixelBits;
        pProperties->limits.minMemoryMapAlignment = vk_physical_device_properties.limits.minMemoryMapAlignment;
        pProperties->limits.minTexelBufferOffsetAlignment =
            (VkDeviceSize)vk_physical_device_properties.limits.minTexelBufferOffsetAlignment;
        pProperties->limits.minUniformBufferOffsetAlignment =
            (VkDeviceSize)vk_physical_device_properties.limits.minUniformBufferOffsetAlignment;
        pProperties->limits.minStorageBufferOffsetAlignment =
            (VkDeviceSize)vk_physical_device_properties.limits.minStorageBufferOffsetAlignment;
        pProperties->limits.minTexelOffset         = vk_physical_device_properties.limits.minTexelOffset;
        pProperties->limits.maxTexelOffset         = vk_physical_device_properties.limits.maxTexelOffset;
        pProperties->limits.minTexelGatherOffset   = vk_physical_device_properties.limits.minTexelGatherOffset;
        pProperties->limits.maxTexelGatherOffset   = vk_physical_device_properties.limits.maxTexelGatherOffset;
        pProperties->limits.minInterpolationOffset = vk_physical_device_properties.limits.minInterpolationOffset;
        pProperties->limits.maxInterpolationOffset = vk_physical_device_properties.limits.maxInterpolationOffset;
        pProperties->limits.subPixelInterpolationOffsetBits =
            vk_physical_device_properties.limits.subPixelInterpolationOffsetBits;
        pProperties->limits.maxFramebufferWidth  = vk_physical_device_properties.limits.maxFramebufferWidth;
        pProperties->limits.maxFramebufferHeight = vk_physical_device_properties.limits.maxFramebufferHeight;
        pProperties->limits.maxFramebufferLayers = vk_physical_device_properties.limits.maxFramebufferLayers;
        pProperties->limits.framebufferColorSampleCounts =
            (VkSampleCountFlags)vk_physical_device_properties.limits.framebufferColorSampleCounts;
        pProperties->limits.framebufferDepthSampleCounts =
            (VkSampleCountFlags)vk_physical_device_properties.limits.framebufferDepthSampleCounts;
        pProperties->limits.framebufferStencilSampleCounts =
            (VkSampleCountFlags)vk_physical_device_properties.limits.framebufferStencilSampleCounts;
        pProperties->limits.framebufferNoAttachmentsSampleCounts =
            (VkSampleCountFlags)vk_physical_device_properties.limits.framebufferNoAttachmentsSampleCounts;
        pProperties->limits.maxColorAttachments = vk_physical_device_properties.limits.maxColorAttachments;
        pProperties->limits.sampledImageColorSampleCounts =
            (VkSampleCountFlags)vk_physical_device_properties.limits.sampledImageColorSampleCounts;
        pProperties->limits.sampledImageIntegerSampleCounts =
            (VkSampleCountFlags)vk_physical_device_properties.limits.sampledImageIntegerSampleCounts;
        pProperties->limits.sampledImageDepthSampleCounts =
            (VkSampleCountFlags)vk_physical_device_properties.limits.sampledImageDepthSampleCounts;
        pProperties->limits.sampledImageStencilSampleCounts =
            (VkSampleCountFlags)vk_physical_device_properties.limits.sampledImageStencilSampleCounts;
        pProperties->limits.storageImageSampleCounts =
            (VkSampleCountFlags)vk_physical_device_properties.limits.storageImageSampleCounts;
        pProperties->limits.maxSampleMaskWords = vk_physical_device_properties.limits.maxSampleMaskWords;
        pProperties->limits.timestampComputeAndGraphics =
            (VkBool32)vk_physical_device_properties.limits.timestampComputeAndGraphics;
        pProperties->limits.timestampPeriod  = vk_physical_device_properties.limits.timestampPeriod;
        pProperties->limits.maxClipDistances = vk_physical_device_properties.limits.maxClipDistances;
        pProperties->limits.maxCullDistances = vk_physical_device_properties.limits.maxCullDistances;
        pProperties->limits.maxCombinedClipAndCullDistances =
            vk_physical_device_properties.limits.maxCombinedClipAndCullDistances;
        pProperties->limits.discreteQueuePriorities = vk_physical_device_properties.limits.discreteQueuePriorities;
        for (uint32_t i = 0; i < 2; i++)
        {
            pProperties->limits.pointSizeRange[i] = vk_physical_device_properties.limits.pointSizeRange[i];
        }
        for (uint32_t i = 0; i < 2; i++)
        {
            pProperties->limits.lineWidthRange[i] = vk_physical_device_properties.limits.lineWidthRange[i];
        }
        pProperties->limits.pointSizeGranularity = vk_physical_device_properties.limits.pointSizeGranularity;
        pProperties->limits.lineWidthGranularity = vk_physical_device_properties.limits.lineWidthGranularity;
        pProperties->limits.strictLines          = (VkBool32)vk_physical_device_properties.limits.strictLines;
        pProperties->limits.standardSampleLocations =
            (VkBool32)vk_physical_device_properties.limits.standardSampleLocations;
        pProperties->limits.optimalBufferCopyOffsetAlignment =
            (VkDeviceSize)vk_physical_device_properties.limits.optimalBufferCopyOffsetAlignment;
        pProperties->limits.optimalBufferCopyRowPitchAlignment =
            (VkDeviceSize)vk_physical_device_properties.limits.optimalBufferCopyRowPitchAlignment;
        pProperties->limits.nonCoherentAtomSize =
            (VkDeviceSize)vk_physical_device_properties.limits.nonCoherentAtomSize;
    }

    RHICommandBuffer*        VulkanRHI::getCurrentCommandBuffer() const { return m_current_command_buffer; }
    RHICommandBuffer* const* VulkanRHI::getCommandBufferList() const { return m_command_buffers; }
    RHICommandPool*          VulkanRHI::getCommandPoor() const { return m_rhi_command_pool; }
    RHIDescriptorPool*       VulkanRHI::getDescriptorPoor() const { return m_descriptor_pool; }
    RHIFence* const*         VulkanRHI::getFenceList() const { return m_rhi_is_frame_in_flight_fences; }
    QueueFamilyIndices       VulkanRHI::getQueueFamilyIndices() const { return m_queue_indices; }
    RHIQueue*                VulkanRHI::getGraphicsQueue() const { return m_graphics_queue; }
    RHIQueue*                VulkanRHI::getComputeQueue() const { return m_compute_queue; }

    RHISwapChainDesc VulkanRHI::getSwapchainInfo()
    {
        RHISwapChainDesc desc;
        desc.image_format = m_swap_chain_format;
        desc.extent       = m_swap_chain_extent;
        desc.viewport     = &m_viewport;
        desc.scissor      = &m_scissor;
        desc.imageViews   = m_swap_chain_image_views;
        return desc;
    }
    RHIDepthImageDesc VulkanRHI::getDepthImageInfo() const
    {
        RHIDepthImageDesc desc;
        desc.depth_image_format = m_depth_image_format;
        desc.depth_image_view   = m_depth_image_view;
        desc.depth_image        = m_depth_image;
        return desc;
    }
    uint8_t VulkanRHI::getMaxFramesInFlight() const { return k_max_frames_in_flight; }
    uint8_t VulkanRHI::getCurrentFrameIndex() const { return m_current_frame_index; }
    void    VulkanRHI::setCurrentFrameIndex(uint8_t index) { m_current_frame_index = index; }


    RHICommandBuffer* VulkanRHI::beginSingleTimeCommands()
    {
        VkCommandBufferAllocateInfo vk_command_buffer_allocate_info {};
        vk_command_buffer_allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        vk_command_buffer_allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        vk_command_buffer_allocate_info.commandPool        = ((VulkanCommandPool*)m_rhi_command_pool)->getResource();
        vk_command_buffer_allocate_info.commandBufferCount = 1;

        VkCommandBuffer command_buffer;
        vkAllocateCommandBuffers(m_device, &vk_command_buffer_allocate_info, &command_buffer);

        VkCommandBufferBeginInfo vk_command_buffer_begin_info {};
        vk_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vk_command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        _vkBeginCommandBuffer(command_buffer, &vk_command_buffer_begin_info);

        RHICommandBuffer* rhi_command_buffer = new VulkanCommandBuffer();
        ((VulkanCommandBuffer*)rhi_command_buffer)->setResource(command_buffer);
        return rhi_command_buffer;
    }

    void VulkanRHI::endSingleTimeCommands(RHICommandBuffer* command_buffer
    )
    {
        VkCommandBuffer vk_command_buffer = ((VulkanCommandBuffer*)command_buffer)->getResource();
        _vkEndCommandBuffer(vk_command_buffer);

        VkSubmitInfo vk_submit_info {};
        vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        vk_submit_info.commandBufferCount = 1;
        vk_submit_info.pCommandBuffers    = &vk_command_buffer;

        vkQueueSubmit(((VulkanQueue*)m_graphics_queue)->getResource(),1, &vk_submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(((VulkanQueue*)m_graphics_queue)->getResource());

        vkFreeCommandBuffers(m_device, ((VulkanCommandPool*)m_rhi_command_pool)->getResource(), 1, &vk_command_buffer);

        delete (command_buffer);

    }

    bool VulkanRHI::prepareBeforePass(std::function<void()> pass_update_after_recreate_swapchain)
    {
        VkResult acquire_image_result =
            vkAcquireNextImageKHR(m_device,
                                  m_swap_chain,
                                  UINT64_MAX,
                                  m_image_available_for_render_semaphores[m_current_frame_index],
                                  VK_NULL_HANDLE,
                                  &m_current_swapchain_image_index);

        if (VK_ERROR_OUT_OF_DATE_KHR == acquire_image_result)
        {
            recreateSwapchain();
            pass_update_after_recreate_swapchain();
            return RHI_SUCCESS;
        }
        if (VK_SUBOPTIMAL_KHR == acquire_image_result)
        {
            recreateSwapchain();
            pass_update_after_recreate_swapchain();

            // NULL submit to waite semaphore
            VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};
            VkSubmitInfo         submit_info   = {};
            submit_info.sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.waitSemaphoreCount     = 1;
            submit_info.pWaitSemaphores        = &m_image_available_for_render_semaphores[m_current_frame_index];
            submit_info.pWaitDstStageMask      = wait_stages;
            submit_info.commandBufferCount     = 0;
            submit_info.pCommandBuffers        = NULL;
            submit_info.signalSemaphoreCount   = 0;
            submit_info.pSignalSemaphores      = NULL;

            VkResult res_rest_fences = vkResetFences(m_device, 1, &m_is_frame_in_flight_fences[m_current_frame_index]);
            if (VK_SUCCESS != res_rest_fences)
            {
                LOG_ERROR("_vkResetFences failed!");
                return false;
            }

            VkResult res_queue_submit = vkQueueSubmit(((VulkanQueue*)m_graphics_queue)->getResource(),
                                                      1,
                                                      &submit_info,
                                                      m_is_frame_in_flight_fences[m_current_frame_index]);
            if (VK_SUCCESS != res_queue_submit)
            {
                LOG_ERROR("vkQueueSubmit failed!");
                return false;
            }
            m_current_frame_index = (m_current_frame_index + 1) % k_max_frames_in_flight;
            return RHI_SUCCESS;
        }
        if (VK_SUCCESS != acquire_image_result)
        {
            LOG_ERROR("vkAcquireNextImageKHR failed!");
            return false;
        }

        // begein command buffer
        VkCommandBufferBeginInfo vk_command_buffer_begin_info {};
        vk_command_buffer_begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vk_command_buffer_begin_info.flags            = 0;
        vk_command_buffer_begin_info.pInheritanceInfo = nullptr;

        VkResult res_begin_command_buffer =
            _vkBeginCommandBuffer(m_vk_command_buffers[m_current_frame_index], &vk_command_buffer_begin_info);

        if (VK_SUCCESS != res_begin_command_buffer)
        {
            LOG_ERROR("_vkBeginCommandBuffer failed!");
            return false;
        }
        return false;
    }

    void VulkanRHI::submitRendering(std::function<void()> pass_update_after_recreate_swapchain)
    {
        // end command_buffer
        VkResult res_end_command_buffer = _vkEndCommandBuffer(m_vk_command_buffers[m_current_frame_index]);
        if (VK_SUCCESS != res_end_command_buffer)
        {
            LOG_ERROR("_vkEndCommandBuffer failed!");
            return;
        }

        VkSemaphore vk_semaphore[2] = {
            ((VulkanSemaphore*)m_image_available_for_texturescopy_semaphores[m_current_frame_index])->getResource(),
            m_image_finished_for_presentation_semaphores[m_current_frame_index]};

        // submit command buffer
        VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo         submit_info {};
        submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pWaitSemaphores      = &m_image_available_for_render_semaphores[m_current_frame_index];
        submit_info.pWaitDstStageMask    = wait_stages;
        submit_info.commandBufferCount   = 1;
        submit_info.pCommandBuffers      = &m_vk_command_buffers[m_current_frame_index];
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores    = vk_semaphore;

        VkResult res_reset_fences = _vkResetFences(m_device, 1, &m_is_frame_in_flight_fences[m_current_frame_index]);

        if (VK_SUCCESS != res_reset_fences)
        {
            LOG_ERROR("_vkResetFences failed!");
            return;
        }
        VkResult res_queue_submit = vkQueueSubmit(((VulkanQueue*)m_graphics_queue)->getResource(),
                                                  1,
                                                  &submit_info,
                                                  m_is_frame_in_flight_fences[m_current_frame_index]);

        if (VK_SUCCESS != res_queue_submit)
        {
            LOG_ERROR("vkQueueSubmit failed!");
            return;
        }

        // present swapchain
        VkPresentInfoKHR present_info   = {};
        present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores    = &m_image_finished_for_presentation_semaphores[m_current_frame_index];
        present_info.swapchainCount     = 1;
        present_info.pSwapchains        = &m_swap_chain;
        present_info.pImageIndices      = &m_current_swapchain_image_index;

        VkResult present_result = vkQueuePresentKHR(m_present_queue, &present_info);
        if (VK_ERROR_OUT_OF_DATE_KHR == present_result || VK_SUBOPTIMAL_KHR == present_result)
        {
            recreateSwapchain();
            pass_update_after_recreate_swapchain();
        }
        else
        {
            if (VK_SUCCESS != present_result)
            {
                LOG_ERROR("vkQueuePresentKHR failed!");
                return;
            }
        }

        m_current_frame_index = (m_current_frame_index + 1) % k_max_frames_in_flight;
    }

    void VulkanRHI::pushEvent(RHICommandBuffer* commond_buffer, const char* name, const float* color)
    {
        if (m_enable_debug_utils_label)
        {
            VkDebugUtilsLabelEXT vk_debug_utils_label {};
            vk_debug_utils_label.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            vk_debug_utils_label.pNext      = nullptr;
            vk_debug_utils_label.pLabelName = name;
            for (int i = 0; i < 4; ++i)
            {
                vk_debug_utils_label.color[i] = color[i];
            }
            _vkCmdBeginDebugUtilsLabelEXT(((VulkanCommandBuffer*)commond_buffer)->getResource(), &vk_debug_utils_label);
        }
    }

    void VulkanRHI::popEvent(RHICommandBuffer* commond_buffer
    )
    {
        if (m_enable_debug_utils_label)
        {
            _vkCmdEndDebugUtilsLabelEXT(((VulkanCommandBuffer*)commond_buffer)->getResource());
        }
    }

    void VulkanRHI::destroyDefaultSampler(RHIDefaultSamplerType type
    )
    {
        switch (type)
        {
            case Default_Sampler_Linear:
                VulkanUtil::destroyLinearSampler(m_device);
                delete (m_linear_sampler);
                break;
            case Default_Sampler_Nearest:
                VulkanUtil::destroyNearestSampler(m_device);
                delete (m_nearest_sampler);
                break;
            default:
                break;
        }
    }

    void VulkanRHI::destroyMipmappedSampler()
    {
        VulkanUtil::destroyMipmappedSampler(m_device);

        for (auto sampler : m_mipmap_sampler_map)
        {
            delete sampler.second;
        }
        m_mipmap_sampler_map.clear();
    }

    void VulkanRHI::destroyShaderModule(RHIShader* shader_module
    )
    {
        vkDestroyShaderModule(m_device, ((VulkanShader*)shader_module)->getResource(), nullptr);

        delete (shader_module);
    }

    void VulkanRHI::destroySemaphore(RHISemaphore* semaphore
    )
    {
        vkDestroySemaphore(m_device, ((VulkanSemaphore*)semaphore)->getResource(), nullptr);
    }

    void VulkanRHI::destroySampler(RHISampler* sampler
    )
    {
        vkDestroySampler(m_device, ((VulkanSampler*)sampler)->getResource(), nullptr);
    }

    void VulkanRHI::destroyInstance(RHIInstance* instance
    )
    {
        vkDestroyInstance(((VulkanInstance*)instance)->getResource(), nullptr);
    }

    void VulkanRHI::destroyImageView(RHIImageView* image_view
    )
    {
        vkDestroyImageView(m_device, ((VulkanImageView*)image_view)->getResource(), nullptr);
    }

    void VulkanRHI::destroyImage(RHIImage* image
    )
    {
        vkDestroyImage(m_device, ((VulkanImage*)image)->getResource(), nullptr);
    }

    void VulkanRHI::destroyFramebuffer(RHIFramebuffer* framebuffer
    )
    {
        vkDestroyFramebuffer(m_device, ((VulkanFramebuffer*)framebuffer)->getResource(), nullptr);
    }

    void VulkanRHI::destroyFence(RHIFence* fence
    )
    {
        vkDestroyFence(m_device, ((VulkanFence*)fence)->getResource(), nullptr);
    }

    void VulkanRHI::destroyDevice() { vkDestroyDevice(m_device, nullptr); }

    void VulkanRHI::destroyCommandPool(RHICommandPool* command_pool
    )
    {
        vkDestroyCommandPool(m_device, ((VulkanCommandPool*)command_pool)->getResource(), nullptr);
    }

    void VulkanRHI::destroyBuffer(RHIBuffer*& buffer
    )
    {
        vkDestroyBuffer(m_device, ((VulkanBuffer*)buffer)->getResource(), nullptr);
        RHI_DELETE_PTR(buffer);
    }

    void VulkanRHI::freeCommandBuffers(RHICommandPool* command_pool,
	    uint32_t command_buffer_count,
	    RHICommandBuffer* p_command_buffers
    )
    {
        VkCommandBuffer vk_command_buffer = ((VulkanCommandBuffer*)p_command_buffers)->getResource();
        vkFreeCommandBuffers(
            m_device, ((VulkanCommandPool*)command_pool)->getResource(), command_buffer_count, &vk_command_buffer);
    }

    void VulkanRHI::freeMemory(RHIDeviceMemory*& memory
    )
    {
        vkFreeMemory(m_device, ((VulkanDeviceMemory*)memory)->getResource(), nullptr);
        RHI_DELETE_PTR(memory);
    }

    bool VulkanRHI::mapMemory(RHIDeviceMemory* memory,
	    RHIDeviceSize offset,
	    RHIDeviceSize size,
	    RHIMemoryMapFlags flags,
	    void** pp_data
    )
    {
        VkResult result = vkMapMemory(
            m_device, ((VulkanDeviceMemory*)memory)->getResource(), offset, size, (VkMemoryMapFlags)flags, pp_data);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("vkMapMemory failed!");
            return false;
        }
    }

    void VulkanRHI::unmapMemory(RHIDeviceMemory* memory
    )
    {
        vkUnmapMemory(m_device, ((VulkanDeviceMemory*)memory)->getResource());
    }

    void VulkanRHI::invalidateMappedMemoryRanges(void* p_next,
	    RHIDeviceMemory* memory,
	    RHIDeviceSize offset,
	    RHIDeviceSize size
    )
    {
        VkMappedMemoryRange mapped_range {};
        mapped_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mapped_range.memory = ((VulkanDeviceMemory*)memory)->getResource();
        mapped_range.offset = offset;
        mapped_range.size   = size;
        vkInvalidateMappedMemoryRanges(m_device, 1, &mapped_range);
    }

    void VulkanRHI::flushMappedMemoryRanges(void* p_next,
	    RHIDeviceMemory* memory,
	    RHIDeviceSize offset,
	    RHIDeviceSize size
    )
    {
        VkMappedMemoryRange mapped_range {};
        mapped_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mapped_range.memory = ((VulkanDeviceMemory*)memory)->getResource();
        mapped_range.offset = offset;
        mapped_range.size   = size;
        vkFlushMappedMemoryRanges(m_device, 1, &mapped_range);
    }

    void VulkanRHI::cmdBindVertexBuffersPFN(RHICommandBuffer* command_buffer,
                                            uint32_t first_binding,
                                            uint32_t binding_count,
                                            RHIBuffer* const* p_buffers,
                                            const RHIDeviceSize* p_offsets
    )
    {
        // buffer
        int                   buffer_size = binding_count;
        std::vector<VkBuffer> vk_buffer_list(buffer_size);
        for (int i = 0; i < buffer_size; ++i)
        {
            const auto& rhi_buffer_element = p_buffers[i];
            auto&       vk_buffer_element  = vk_buffer_list[i];

            vk_buffer_element = ((VulkanBuffer*)rhi_buffer_element)->getResource();
        };

        // offset
        int                       offset_size = binding_count;
        std::vector<VkDeviceSize> vk_device_size_list(offset_size);
        for (int i = 0; i < offset_size; ++i)
        {
            const auto& rhi_offset_element = p_offsets[i];
            auto&       vk_offset_element  = vk_device_size_list[i];

            vk_offset_element = rhi_offset_element;
        };

        return _vkCmdBindVertexBuffers(((VulkanCommandBuffer*)command_buffer)->getResource(),
                                       first_binding,
                                       binding_count,
                                       vk_buffer_list.data(),
                                       vk_device_size_list.data());
    }

    void VulkanRHI::cmdBindDescriptorSetsPFN(RHICommandBuffer* command_buffer,
	    RHIPipelineBindPoint pipeline_bind_point,
	    RHIPipelineLayout* layout,
	    uint32_t first_set,
	    uint32_t descriptor_set_count,
	    const RHIDescriptorSet* const* p_descriptor_sets,
	    uint32_t dynamic_offset_count,
	    const uint32_t* p_dynamic_offsets
    ) {}

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

    void VulkanRHI::createWindowSurface()
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
        QueueFamilyIndices queue_indices = findQueueFamilies(device);

        bool extension_supported = checkDeviceExtensionSupport(device);

        bool is_swapchain_adequate = false;
        if (extension_supported)
        {
            SwapChainSupportDetails swapchain_support = querySwapChainSupport(device);
            is_swapchain_adequate =
                !swapchain_support.formats.empty() && !swapchain_support.presentModes.empty();
        }

        VkPhysicalDeviceFeatures physical_device_features {};
        vkGetPhysicalDeviceFeatures(device, &physical_device_features);

        return queue_indices.isComplete() && is_swapchain_adequate && physical_device_features.samplerAnisotropy;
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
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

        // including the type of operations that are supported and the number of queues that can be
        // created based on that family.

        uint32_t i = 0;
        for (const auto& queue_family : queue_families)
        {
            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphics_family = i;
            }

            if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT) // if support compute command queue
            {
                indices.compute_family = i;
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
        m_queue_indices = findQueueFamilies(m_physical_device);

        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        std::set<uint32_t>                   unique_queue_families {m_queue_indices.graphics_family.value(),
                                                  m_queue_indices.present_family.value(),
                                                  m_queue_indices.compute_family.value()};

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

        physical_device_features.samplerAnisotropy = VK_TRUE;
        //support inefficinet readback storage buffer
        physical_device_features.fragmentStoresAndAtomics = VK_TRUE;
        //support independent  storage buffer
        physical_device_features.independentBlend = VK_TRUE;
        //support geometry shader
        if (m_enable_point_light_shadow)
        {
            physical_device_features.geometryShader = VK_TRUE;
        }

        //device create info
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
        VkQueue vk_graphics_queue;
        vkGetDeviceQueue(m_device, m_queue_indices.graphics_family.value(), 0, &vk_graphics_queue);
        m_graphics_queue = new VulkanQueue();
        ((VulkanQueue*)m_graphics_queue)->setResource(vk_graphics_queue);

        vkGetDeviceQueue(m_device, m_queue_indices.present_family.value(), 0, &m_present_queue);

               VkQueue vk_compute_queue;
        vkGetDeviceQueue(m_device, m_queue_indices.graphics_family.value(), 0, &vk_compute_queue);
        m_compute_queue = new VulkanQueue();
        ((VulkanQueue*)m_compute_queue)->setResource(vk_graphics_queue);

                // more efficient pointer
        _vkResetCommandPool     = (PFN_vkResetCommandPool)vkGetDeviceProcAddr(m_device, "vkResetCommandPool");
        _vkBeginCommandBuffer   = (PFN_vkBeginCommandBuffer)vkGetDeviceProcAddr(m_device, "vkBeginCommandBuffer");
        _vkEndCommandBuffer     = (PFN_vkEndCommandBuffer)vkGetDeviceProcAddr(m_device, "vkEndCommandBuffer");
        _vkCmdBeginRenderPass   = (PFN_vkCmdBeginRenderPass)vkGetDeviceProcAddr(m_device, "vkCmdBeginRenderPass");
        _vkCmdNextSubpass       = (PFN_vkCmdNextSubpass)vkGetDeviceProcAddr(m_device, "vkCmdNextSubpass");
        _vkCmdEndRenderPass     = (PFN_vkCmdEndRenderPass)vkGetDeviceProcAddr(m_device, "vkCmdEndRenderPass");
        _vkCmdBindPipeline      = (PFN_vkCmdBindPipeline)vkGetDeviceProcAddr(m_device, "vkCmdBindPipeline");
        _vkCmdSetViewport       = (PFN_vkCmdSetViewport)vkGetDeviceProcAddr(m_device, "vkCmdSetViewport");
        _vkCmdSetScissor        = (PFN_vkCmdSetScissor)vkGetDeviceProcAddr(m_device, "vkCmdSetScissor");
        _vkWaitForFences        = (PFN_vkWaitForFences)vkGetDeviceProcAddr(m_device, "vkWaitForFences");
        _vkResetFences          = (PFN_vkResetFences)vkGetDeviceProcAddr(m_device, "vkResetFences");
        _vkCmdDrawIndexed       = (PFN_vkCmdDrawIndexed)vkGetDeviceProcAddr(m_device, "vkCmdDrawIndexed");
        _vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)vkGetDeviceProcAddr(m_device, "vkCmdBindVertexBuffers");
        _vkCmdBindIndexBuffer   = (PFN_vkCmdBindIndexBuffer)vkGetDeviceProcAddr(m_device, "vkCmdBindIndexBuffer");
        _vkCmdBindDescriptorSets =
            (PFN_vkCmdBindDescriptorSets)vkGetDeviceProcAddr(m_device, "vkCmdBindDescriptorSets");
        _vkCmdClearAttachments = (PFN_vkCmdClearAttachments)vkGetDeviceProcAddr(m_device, "vkCmdClearAttachments");

    	m_depth_image_format = (RHIFormat)findDepthFormat();
    }

    void VulkanRHI::createSwapchain()
    {
        SwapChainSupportDetails swap_chain_support = querySwapChainSupport(m_physical_device);

        VkSurfaceFormatKHR surface_format = chooseSwapchainSurfaceFormatFromDetails(swap_chain_support.formats);
        VkPresentModeKHR   present_mode   = chooseSwapchainPresentModeFromDetails(swap_chain_support.presentModes);
        VkExtent2D         extent         = chooseSwapchainExtentFromDetails(swap_chain_support.capabilities);

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

        uint32_t queue_family_indices[] = {m_queue_indices.graphics_family.value(),
                                           m_queue_indices.present_family.value()};

        // use concurrent
        if (m_queue_indices.graphics_family != m_queue_indices.present_family)
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

        m_swap_chain_format = (RHIFormat)surface_format.format;
        m_swap_chain_extent.width = extent.width;
        m_swap_chain_extent.height = extent.height;

        m_scissor = {{0, 0}, {m_swap_chain_extent.width, m_swap_chain_extent.height}};
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
        uint32_t present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count, nullptr);
        if (present_mode_count != 0)
        {
            details.presentModes.resize(present_mode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                device, m_surface, &present_mode_count, details.presentModes.data());
        }


        return details;
    }


    void VulkanRHI::createCommandPool()
    {
        //default graphics command pool
	    {
            m_rhi_command_pool = new VulkanCommandPool();
            VkCommandPool vk_command_pool;
            VkCommandPoolCreateInfo vk_command_pool_create_info {};
            vk_command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            vk_command_pool_create_info.pNext = NULL;
            vk_command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            vk_command_pool_create_info.queueFamilyIndex = m_queue_indices.graphics_family.value();

            if (vkCreateCommandPool(m_device,&vk_command_pool_create_info,nullptr,&vk_command_pool)!=VK_SUCCESS)
            {
	            LOG_ERROR("failed to create command pool!");
            }

            ((VulkanCommandPool*)m_rhi_command_pool)->setResource(vk_command_pool);
	    }

        //other
	    {
            VkCommandPoolCreateInfo vk_command_pool_create_info {};
            vk_command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            vk_command_pool_create_info.pNext = NULL;
            vk_command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            vk_command_pool_create_info.queueFamilyIndex = m_queue_indices.graphics_family.value();


            for (uint32_t i=0;i<k_max_frames_in_flight;++i)
            {
                if (vkCreateCommandPool(m_device, &vk_command_pool_create_info, nullptr, &m_command_pools[i]) !=
                    VK_SUCCESS)
                {
                    LOG_ERROR("failed to create command pool!");
                }
            }
	    }
    }
    void VulkanRHI::createCommandBuffers()
    {
        VkCommandBufferAllocateInfo vk_command_buffer_allocate_info {};
        vk_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        vk_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        vk_command_buffer_allocate_info.commandBufferCount = 1U;

        for (uint32_t i=0;i<k_max_frames_in_flight;++i)
        {
            vk_command_buffer_allocate_info.commandPool = m_command_pools[i];
            VkCommandBuffer vk_command_buffer;
            if (vkAllocateCommandBuffers(m_device,&vk_command_buffer_allocate_info,&vk_command_buffer)!=VK_SUCCESS)
            {
                LOG_ERROR("vk allocate command buffers!");
            }
            m_vk_command_buffers[i] = vk_command_buffer;
            m_command_buffers[i]    = new VulkanCommandBuffer;
            ((VulkanCommandBuffer*)m_command_buffers[i] )->setResource(vk_command_buffer);
        }
    }

    void VulkanRHI::createDescriptorPool()
    {
        // Since DescriptorSet should be treated as asset in Vulkan, DescriptorPool
        // should be big enough, and thus we can sub-allocate DescriptorSet from
        // DescriptorPool merely as we sub-allocate Buffer/Image from DeviceMemory.

        VkDescriptorPoolSize pool_sizes[7];
        pool_sizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        pool_sizes[0].descriptorCount = 3 + 2 + 2 + 1 + 1 + 1 + 3 + 3;
        pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        pool_sizes[1].descriptorCount = 1 + 1 + 1 * m_max_vertex_blending_mesh_count;
        pool_sizes[2].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_sizes[2].descriptorCount = 1 * m_max_material_count;
        pool_sizes[3].type             = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes[3].descriptorCount = 3 + 5 * m_max_material_count + 1 + 1; // ImGui_ImplVulkan_CreateDeviceObjects
        pool_sizes[4].type             = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        pool_sizes[4].descriptorCount  = 4 + 1 + 1 + 2;
        pool_sizes[5].type             = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        pool_sizes[5].descriptorCount  = 3;
        pool_sizes[6].type             = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        pool_sizes[6].descriptorCount  = 1;

        VkDescriptorPoolCreateInfo vk_descriptor_pool_create_info {};
        vk_descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        vk_descriptor_pool_create_info.poolSizeCount = std::size(pool_sizes);
        vk_descriptor_pool_create_info.pPoolSizes    = pool_sizes;
        vk_descriptor_pool_create_info.maxSets =
            1 + 1 + 1 + m_max_material_count + m_max_vertex_blending_mesh_count + 1 + 1;
			// // +skybox + axis descriptor set

        vk_descriptor_pool_create_info.flags = 0U;

        if (vkCreateDescriptorPool(m_device, &vk_descriptor_pool_create_info, nullptr, &m_vk_descriptor_pool)!=VK_SUCCESS)
        {
            LOG_ERROR("creaete descirpt pool");
        }

        m_descriptor_pool = new VulkanDescriptorPool();
        ((VulkanDescriptorPool*)m_descriptor_pool)->setResource(m_vk_descriptor_pool);
    }

    void VulkanRHI::createSwapchainImageViews()
    {
        m_swap_chain_image_views.resize(m_swap_chain_images.size());

        for (size_t i = 0; i < m_swap_chain_images.size(); ++i)
        {
            VkImageView vk_image_view;
            vk_image_view = VulkanUtil::createImageView(m_device,
                                                        m_swap_chain_images[i],
                                                        (VkFormat)m_swap_chain_format,
                                                        VK_IMAGE_ASPECT_COLOR_BIT,
                                                        VK_IMAGE_VIEW_TYPE_2D,
                                                        1,
                                                        1);
            m_swap_chain_image_views[i] = new VulkanImageView();
            ((VulkanImageView*)m_swap_chain_image_views[i])->setResource(vk_image_view);
        }
    }

    void VulkanRHI::clearSwapchain()
    {
        for (auto image_view : m_swap_chain_image_views)
        {
            vkDestroyImageView(m_device, ((VulkanImageView*)image_view)->getResource(), nullptr);
        }
        vkDestroySwapchainKHR(m_device, m_swap_chain, nullptr);
    }

    void VulkanRHI::createFramebufferImageAndView()
    {
        VulkanUtil::createImage(m_physical_device,
                                m_device,
                                m_swap_chain_extent.width,
                                m_swap_chain_extent.height,
                                (VkFormat)m_depth_image_format,
                                VK_IMAGE_TILING_OPTIMAL,
                                VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                ((VulkanImage*)m_depth_image)->getResource(),
                                m_depth_image_memory,
                                0,
                                1,
                                1);

        ((VulkanImageView*)m_depth_image_view)
            ->setResource(VulkanUtil::createImageView(m_device,
                                                      ((VulkanImage*)m_depth_image)->getResource(),
                                                      (VkFormat)m_depth_image_format,
                                                      VK_IMAGE_ASPECT_DEPTH_BIT,
                                                      VK_IMAGE_VIEW_TYPE_2D,
                                                      1,
                                                      1));

    }

    // semaphore : signal an image is ready for rendering // ready for presentation
    // (m_vulkan_context._swapchain_images --> semaphores, fences)
    void VulkanRHI::createSyncPrimitive()
    {
	    VkSemaphoreCreateInfo vk_semaphore_create_info {};
        vk_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo vk_fence_create_info {};
        vk_fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        vk_fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;      //the fence is initialized as signaled
        for (uint32_t i=0;i<k_max_frames_in_flight;++i)
        {
            m_image_available_for_texturescopy_semaphores[i] = new RHISemaphore();
            if (vkCreateSemaphore(
                    m_device, &vk_semaphore_create_info, nullptr, &m_image_available_for_render_semaphores[i]) !=
                    VK_SUCCESS ||
                vkCreateSemaphore(
                    m_device, &vk_semaphore_create_info, nullptr, &m_image_finished_for_presentation_semaphores[i]) !=
                    VK_SUCCESS ||
                vkCreateSemaphore(
                    m_device,
                    &vk_semaphore_create_info,
                    nullptr,
                    &((VulkanSemaphore*)m_image_available_for_texturescopy_semaphores[i])->getResource()) !=
                    VK_SUCCESS ||
                vkCreateFence(m_device, &vk_fence_create_info, nullptr, &m_is_frame_in_flight_fences[i]) != VK_SUCCESS)
            {
                LOG_ERROR("vk create semaphor & fence");
            }

            m_rhi_is_frame_in_flight_fences[i] = new VulkanFence();
            ((VulkanFence*)m_rhi_is_frame_in_flight_fences[i])->setResource(m_is_frame_in_flight_fences[i]);
        }
    }
    void VulkanRHI::createAssetAllocator()
    {
        VmaVulkanFunctions vulkan_functions {};
        vulkan_functions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkan_functions.vkGetDeviceProcAddr   = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo vma_allocator_create_info {};
        vma_allocator_create_info.vulkanApiVersion = m_vulkan_version;
        vma_allocator_create_info.physicalDevice   = m_physical_device;
        vma_allocator_create_info.device           = m_device;
        vma_allocator_create_info.instance         = m_instance;
        vma_allocator_create_info.pVulkanFunctions = &vulkan_functions;

        vmaCreateAllocator(&vma_allocator_create_info, &m_assets_allocator);
    }

    void VulkanRHI::setupDebugMessenger()
    {
        if (m_enable_validation_layers)
        {
            VkDebugUtilsMessengerCreateInfoEXT create_info{};
            populateDebugMessengerCreateInfo(create_info);

            if (createDebugUtilsMessengerExt(
                    m_instance, &create_info, nullptr, &m_debug_messenger) != VK_SUCCESS)
            {
                LOG_ERROR("failed to set up debug messenger!");
            }
        }

    	if (m_enable_debug_utils_label)
        {
            _vkCmdBeginDebugUtilsLabelEXT =
                (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(m_instance, "vkCmdBeginDebugUtilsLabelEXT");
            _vkCmdEndDebugUtilsLabelEXT =
                (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(m_instance, "vkCmdEndDebugUtilsLabelEXT");
        }
    }

    VkFormat VulkanRHI::findDepthFormat()
    {
        return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                   VK_IMAGE_TILING_OPTIMAL,
                                   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    VkFormat VulkanRHI::findSupportedFormat(const std::vector<VkFormat>& candidates,
	    VkImageTiling tiling,
	    VkFormatFeatureFlags features
    )
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_physical_device, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
        	else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        LOG_ERROR("findSupportedFormat failed");
        return VkFormat();
    }

    bool VulkanRHI::checkValidationLayerSupport()
    {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        for (const char* layer_name : m_validation_layer)
        {
            bool layer_found = false;
            for (const auto& layer_properties : available_layers)
            {
                if (strcmp(layer_name, layer_properties.layerName) == 0)
                {
                    layer_found = true;
                    break;
                }
            }

            if (!layer_found)
            {
                return false;
            }
        }

        return true;
    }

    std::vector<const char*> VulkanRHI::getRequiredExtensions()
    {
        uint32_t     glfw_extension_count = 0;
        const char** glfw_extensions;
        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

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

    VkResult VulkanRHI::createDebugUtilsMessengerExt(
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

    void VulkanRHI::destroyDebugUtilsMessengerExt(VkInstance                   instance,
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

    VkSurfaceFormatKHR VulkanRHI::chooseSwapchainSurfaceFormatFromDetails(const std::vector<VkSurfaceFormatKHR>& available_surface_formats
    )
    {
        for (const auto& surface_format : available_surface_formats)
        {
            // TODO: select the VK_FORMAT_B8G8R8A8_SRGB surface format,
            // there is no need to do gamma correction in the fragment shader
            if (surface_format.format == VK_FORMAT_B8G8R8A8_UNORM &&
                surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return surface_format;
            }
        }
        return available_surface_formats[0];
    }

    VkPresentModeKHR VulkanRHI::chooseSwapchainPresentModeFromDetails(const std::vector<VkPresentModeKHR>& available_present_modes)
    {
        for (VkPresentModeKHR present_mode : available_present_modes)
        {
            if (VK_PRESENT_MODE_MAILBOX_KHR == present_mode)
            {
                return VK_PRESENT_MODE_MAILBOX_KHR;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanRHI::chooseSwapchainExtentFromDetails(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(m_window, &width, &height);

            VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            actual_extent.width =
                std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actual_extent.height =
                std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actual_extent;
        }
    }


    void VulkanRHI::clear()
    {
        if (m_enable_validation_layers)
        {
            destroyDebugUtilsMessengerExt(m_instance, m_debug_messenger, nullptr);
        }
        vkDestroyDevice(m_device, nullptr);
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        vkDestroyInstance(m_instance, nullptr);
    }
}   // namespace Bocchi