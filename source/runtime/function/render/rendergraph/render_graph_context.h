#pragma once
#include <nvrhi/utils.h>
#include "runtime/function/render/RHI/rhi.h"
#include "runtime/function/render/rendergraph/render_graph_black_board.h"
#include "runtime/function/render/rendergraph/render_graph_resource_id.h"
#include "runtime/function/render/rendergraph/render_graph_resource_name.h"

#if RG_DEBUG
#endif

namespace bocchi
{
    class RenderGraphPassBase;
    class RenderGraph;

    template<RenderGraphRescourceType Rescourcetype>
    struct RenderGraphResouceTraits;

    template<>
    struct RenderGraphResouceTraits<RenderGraphRescourceType::Texture>
    {
        using Resource     = nvrhi::TextureHandle;
        using ResourceDesc = nvrhi::TextureDesc;
    };

    template<>
    struct RenderGraphResouceTraits<RenderGraphRescourceType::Buffer>
    {
        using Resource     = nvrhi::BufferHandle;
        using ResourceDesc = nvrhi::BufferDesc;
    };

    struct RenderGraphResource
    {
        RenderGraphResource(size_t id, bool imported, char const* name) :
            id(id), imported(imported), version(0), ref_count(0), name(name)
        {}

        size_t id;
        bool   imported;
        size_t version;
        size_t ref_count;

        RenderGraphPassBase* writer       = nullptr;
        RenderGraphPassBase* last_used_by = nullptr;
        char const*          name         = "";
    };

    template<RenderGraphRescourceType ResourceType>
    struct TypeRenderGraphResource : RenderGraphResource
    {
        using Resouce      = typename RenderGraphResouceTraits<ResourceType>::Resource;
        using ResourceDesc = typename RenderGraphResouceTraits<ResourceType>::ResourceDesc;

        TypeRenderGraphResource(size_t id, Resouce* resource, char const* name = "") :
            RenderGraphResource(id, true, name), resource(resource), resource_desc(resource->Get()->getDesc())
        {}

        TypeRenderGraphResource(size_t id, const ResourceDesc& resource, char const* name = "") :
            RenderGraphResource(id, true, name), resource(nullptr), resource_desc(resource)
        {}

    	void SetName() const
        {
#if RG_DEBUG
            ASSERT(resource != nullptr && "Call SetDebugName at allocation/creation of a resource");
            resource_desc.debugName = std::string(name);
#endif
        }
        Resouce*     resource;
        ResourceDesc resource_desc;
    };

    using RenderTexture=TypeRenderGraphResource<RenderGraphRescourceType::Texture>;
    using RenderBuffer=TypeRenderGraphResource<RenderGraphRescourceType::Buffer>;

    class RenderGraphContext
    {
        friend RenderGraph;
    public:
        RenderGraphContext()                          = delete;
        RenderGraphContext(const RenderGraphContext&) = delete;
        RenderGraphContext& operator=(const RenderGraphContext&) = delete;

        RenderGraphBlackBoard& GetBlackBloard();

        nvrhi::TextureHandle GetTexture(RenderGraphTextureId res_id) const;
        nvrhi::BufferHandle  GetBuffer(RenderGraphBufferId res_id) const;

        nvrhi::TextureHandle const GetCopySrcTexture(RenderTextureCopySrcId res_id) const;
        nvrhi::TextureHandle GetCopyDstTexture(RenderTextureCopyDstId res_id) const;
        nvrhi::BufferHandle const  GetCopySrcBuffer(RenderGraphBufferId res_id) const;
        nvrhi::BufferHandle GetCopyDstBuffer(RenderGraphBufferId res_id) const;
        nvrhi::BufferHandle const GetIndirectBuffer(RenderBufferIndirectArgsId res_id) const;
        nvrhi::BufferHandle const GetVertexBuffer(RenderBufferVertexId res_id) const;
        nvrhi::BufferHandle const  GetIndexBuffer(RenderBufferIndexId res_id) const;
        nvrhi::BufferHandle const GetConstantBuffer(RenderBufferConstantId res_id) const;

        nvrhi::DescriptorTableHandle GetRenderTarget(RenderGraphRenderTargetId res_id) const;
        nvrhi::DescriptorTableHandle GetDepthStencil(RenderGraphDepthStencilId res_id) const;
        nvrhi::DescriptorTableHandle GetReadOnlyTexture(RenderGraphTextureReadOnlyId res_id) const;
        nvrhi::DescriptorTableHandle GetReadWriteTexture(RenderGraphTextureReadWriteId res_id) const;
        nvrhi::DescriptorTableHandle GetReadOnlyBuffer(RenderGraphBufferReadOnlyId res_id) const;
        nvrhi::DescriptorTableHandle GetReadWriteBuffer(RenderGraphBufferReadWriteId res_id) const;

    private:
        RenderGraph & m_render_graph_;
        RenderGraphPassBase& m_render_graph_pass_;

        RenderGraphContext(RenderGraph& render_graph, RenderGraphPassBase& render_graph_pass);
    };
} // namespace bocchi
