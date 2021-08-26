#include "htpch.h"
#include "OpenGLGraphicsPipeline.h"

namespace Heart
{
    OpenGLGraphicsPipeline::OpenGLGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo, u32 viewportWidth, u32 viewportHeight)
        : GraphicsPipeline(createInfo)
    {

    }

    OpenGLGraphicsPipeline::~OpenGLGraphicsPipeline()
    {

    }
}