#pragma once
#include "runtime/function/render/rendergraph/render_graph_context.h"
#include "runtime/function/render/rendergraph/render_graph_pass.h"
#include "runtime/function/render/rendergraph/render_graph_resource_name.h"

namespace bocchi
{
    class RenderGraphBuilder
    {
        friend class RenderGraph;

    public:
        RenderGraphBuilder()                                     = delete;
        RenderGraphBuilder(RenderGraphBuilder const&)            = delete;
        RenderGraphBuilder& operator=(RenderGraphBuilder const&) = delete;

        bool IsTextureDeclared(RenderGraphResourceName name) const;
        bool IsBufferDesclared(RenderGraphResourceName name) const;
        void ExportTexture(RenderGraphResourceName name, nvrhi::TextureHandle texture_handle);
        void ExportBuffer(RenderGraphResourceName name, nvrhi::BufferHandle buffer_handle);
        void DeclaredTexture(RenderGraphResourceName name, nvrhi::TextureDesc const& texture_desc);
        void DeclaredBuffer(RenderGraphResourceName name, nvrhi::BufferDesc const& buffer_desc);

        void DummyWriteTexture(RenderGraphResourceName name);
        void DummyReadTexture(RenderGraphResourceName name);
        void DummyWriteBuffer(RenderGraphResourceName name);
        void DummyReadBuffer(RenderGraphResourceName name);

        [[nodiscard]] RenderTextureCopySrcId ReadCopyTexture(RenderGraphResourceName name);
        [[nodiscard]] RenderTextureCopyDstId WriteCopyTexture(RenderGraphResourceName name);

        [[nodiscard]] RenderBufferCopySrcid      ReadCopyBuffer(RenderGraphResourceName name);
        [[nodiscard]] RenderBufferCopyDstId      WriteCopyBuffer(RenderGraphResourceName name);
        [[nodiscard]] RenderBufferIndirectArgsId ReadIndiretArgsBuffer(RenderGraphResourceName name);
        [[nodiscard]] RenderBufferVertexId       ReadVertexBuffer(RenderGraphResourceName name);
        [[nodiscard]] RenderBufferIndexId        ReadIndexBuffer(RenderGraphResourceName name);
        [[nodiscard]] RenderBufferConstantId     ReaConstantIdTexture(RenderGraphResourceName name);

        [[nodiscard]] RenderTextureCopySrcId ReadTexture(RenderGraphResourceName name,
                                                         RenderGarphReadAccess   read_access = ReadAccessAllshader,
                                                         uint32_t                first_mip   = 0,
                                                         uint32_t                mip_count   = -1,
                                                         uint32_t                first_slice = 0,
                                                         uint32_t                slice_count = -1)
        {
            return ReadTextureImpl(
                name, read_access, nvrhi::TextureSubresourceSet {first_mip, mip_count, first_slice, slice_count});
        }
        [[nodiscard]] RenderGraphTextureReadWriteId WriteTexture(RenderGraphResourceName name,
                                                                 uint32_t                first_mip   = 0,
                                                                 uint32_t                mip_count   = -1,
                                                                 uint32_t                first_slice = 0,
                                                                 uint32_t                slice_count = -1)
        {
            return WriteTextureImpl(name,
                                    nvrhi::TextureSubresourceSet {first_mip, mip_count, first_slice, slice_count});
        }
        [[maybe_unused]] RenderGraphRenderTargetId WriteRenderTarget(RenderGraphResourceName      name,
                                                                     RenderGraphLoadStoreAccessOp load_store_op,
                                                                     uint32_t                     first_mip   = 0,
                                                                     uint32_t                     mip_count   = -1,
                                                                     uint32_t                     first_slice = 0,
                                                                     uint32_t                     slice_count = -1)
        {
            return WriteRenderTargetImpl(
                name, load_store_op, nvrhi::TextureSubresourceSet {first_mip, mip_count, first_slice, slice_count});
        }
        [[maybe_unused]] RenderGraphDepthStencilId WriteDepthStencil(RenderGraphResourceName      name,
                                                                     RenderGraphLoadStoreAccessOp load_store_op,
                                                                     uint32_t                     first_mip   = 0,
                                                                     uint32_t                     mip_count   = -1,
                                                                     uint32_t                     first_slice = 0,
                                                                     uint32_t                     slice_count = -1)
        {
            return WriteDepthStencilImpl(name,
                                         load_store_op,
                                         RenderGraphLoadStoreAccessOp::NoAccessWithNoAccess,
                                         nvrhi::TextureSubresourceSet {first_mip, mip_count, first_slice, slice_count});
        }
        [[maybe_unused]] RenderGraphDepthStencilId ReadDepthStencil(RenderGraphResourceName      name,
                                                                    RenderGraphLoadStoreAccessOp load_store_op,
                                                                    uint32_t                     first_mip   = 0,
                                                                    uint32_t                     mip_count   = -1,
                                                                    uint32_t                     first_slice = 0,
                                                                    uint32_t                     slice_count = -1)
        {
            return ReadDepthStencilImpl(name,
                                        load_store_op,
                                        RenderGraphLoadStoreAccessOp::NoAccessWithNoAccess,
                                        nvrhi::TextureSubresourceSet {first_mip, mip_count, first_slice, slice_count});
        }
        [[nodiscard]] RenderGraphBufferReadOnlyId ReadBuffer(RenderGraphResourceName name,
                                                             RenderGarphReadAccess   read_access = ReadAccessAllshader,
                                                             uint32_t                offset      = 0,
                                                             uint32_t                size        = -1)
        {
            return ReadBufferImpl(name, read_access, nvrhi::BufferRange {offset, size});
        }
        [[nodiscard]] RenderGraphBufferReadWriteId
        WriteBuffer(RenderGraphResourceName name, uint32_t offset = 0, uint32_t size = -1)
        {
            return WriteBufferImpl(name, nvrhi::BufferRange {offset, size});
        }
        [[nodiscard]] RenderGraphBufferReadWriteId WriteBuffer(RenderGraphResourceName name,
                                                               RenderGraphResourceName counter_name,
                                                               uint32_t                offset = 0,
                                                               uint32_t                size   = -1)
        {
            return WriteBufferImpl(name, counter_name, nvrhi::BufferRange {offset, size});
        }

        void SetViewPort(uint32_t width, uint32_t height);
        void AddBufferStates(RenderGraphResourceName name, nvrhi::ResourceStates states);
        void AddTexturerStates(RenderGraphResourceName name, nvrhi::ResourceStates states);

    private:
        RenderGraph&         m_render_graph_;
        RenderGraphPassBase& m_render_graph_pass_;

    private:
        RenderGraphBuilder(RenderGraph&, RenderGraphPassBase&);

        [[nodiscard]] RenderTextureCopySrcId        ReadTextureImpl(RenderGraphResourceName             name,
                                                                    RenderGarphReadAccess               read_access,
                                                                    nvrhi::TextureSubresourceSet const& desc);
        [[nodiscard]] RenderGraphTextureReadWriteId WriteTextureImpl(RenderGraphResourceName             name,
                                                                     nvrhi::TextureSubresourceSet const& desc);
        [[maybe_unused]] RenderGraphRenderTargetId  WriteRenderTargetImpl(RenderGraphResourceName      name,
                                                                          RenderGraphLoadStoreAccessOp load_store_op,
                                                                          nvrhi::TextureSubresourceSet const& desc);
        [[maybe_unused]] RenderGraphDepthStencilId
        WriteDepthStencilImpl(RenderGraphResourceName             name,
                              RenderGraphLoadStoreAccessOp        load_store_op,
                              RenderGraphLoadStoreAccessOp        stencil_load_store_op,
                              nvrhi::TextureSubresourceSet const& desc);
        [[maybe_unused]] RenderGraphDepthStencilId
        ReadDepthStencilImpl(RenderGraphResourceName             name,
                             RenderGraphLoadStoreAccessOp        load_store_op,
                             RenderGraphLoadStoreAccessOp        stencil_load_store_op,
                             nvrhi::TextureSubresourceSet const& desc);
        [[nodiscard]] RenderGraphBufferReadOnlyId
        ReadBufferImpl(RenderGraphResourceName name, RenderGarphReadAccess read_access, nvrhi::BufferRange const& desc);
        [[nodiscard]] RenderGraphBufferReadWriteId WriteBufferImpl(RenderGraphResourceName   name,
                                                                   nvrhi::BufferRange const& desc);
        [[nodiscard]] RenderGraphBufferReadWriteId WriteBufferImpl(RenderGraphResourceName   name,
                                                                   RenderGraphResourceName   counter_name,
                                                                   nvrhi::BufferRange const& desc);
    };
} // namespace bocchi
