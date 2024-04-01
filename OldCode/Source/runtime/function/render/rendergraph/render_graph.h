#pragma once

#include "nvrhi/nvrhi.h"
#include "runtime/function/render/rendergraph/render_graph_black_board.h"
#include "runtime/function/render/rendergraph/render_graph_builder.h"
#include "runtime/function/render/rendergraph/render_graph_context.h"
#include "runtime/function/render/rendergraph/render_graph_resource_id.h"
#include <memory>
#include <unordered_set>
#include <vector>


namespace bocchi
{
    class RenderGraph
    {
        friend class RenderGraphBuilder;
        friend class RenderGraphContext;

        class DependencyLevel
        {
            friend RenderGraph;

        public:
            explicit DependencyLevel(RenderGraph& render_graph);

        private:
            RenderGraph& m_render_graph_;
            std::vector<RenderGraphPassBase*> m_passes_;

            std::unordered_set<RenderGraphTextureId> m_texture_creates_;
            std::unordered_set<RenderGraphTextureId> m_texture_reads_;
            std::unordered_set<RenderGraphTextureId> m_texture_writes_;
            std::unordered_set<RenderGraphTextureId> m_texture_destorys_;
            std::unordered_map<RenderGraphTextureId,nvrhi::ResourceStates> m_texture_state_map;

            std::unordered_set<RenderGraphBufferId> m_buffer_creates_;
            std::unordered_set<RenderGraphBufferId> m_buffer__reads_;
            std::unordered_set<RenderGraphBufferId> m_buffer__writes_;
            std::unordered_set<RenderGraphBufferId> m_buffer__destorys_;
            std::unordered_map<RenderGraphBufferId,nvrhi::ResourceStates> m_buffer__state_map;
        };
    }
}