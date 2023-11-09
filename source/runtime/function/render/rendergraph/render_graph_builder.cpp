#include "render_graph_builder.h"
#include "runtime/function/render/rendergraph/render_graph_pass.h"

namespace bocchi
{
    RenderGraphBuilder::RenderGraphBuilder(RenderGraph& render_graph, RenderGraphPassBase& render_graph_pass):
	m_render_graph_(render_graph),m_render_graph_pass_(render_graph_pass){}

    // bool RenderGraphBuilder::IsTextureDeclared(RenderGraphResourceName name)
    // {
    //     return m_render_graph_.IsTextureDeclared(name);
    // }
}
