#include "render_graph_context.h"

namespace bocchi
{
	RenderGraphContext::RenderGraphContext(RenderGraph& render_graph,
		RenderGraphPassBase& render_graph_pass
	):m_render_graph_(render_graph),m_render_graph_pass_(render_graph_pass) {}
}
