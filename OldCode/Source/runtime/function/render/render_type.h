#pragma once
#include <cstdint>

namespace bocchi
{
	enum class RenderPipelineType:uint8_t
	{
		FORWARD_PIPELINE=0,
		DEFERRED_PIPELINE,
        PIPELINE_TYPE_COUNT
	};

}
