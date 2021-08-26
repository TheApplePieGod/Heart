#pragma once

#include "Heart/Renderer/Pipeline.h"

namespace Heart
{
    class OpenGLGraphicsPipeline : public GraphicsPipeline
    {
    public:
        OpenGLGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo, u32 viewportWidth, u32 viewportHeight);
        ~OpenGLGraphicsPipeline() override;
    };
}