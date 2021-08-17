#pragma once

#include "Heart/Renderer/Pipeline.h"

namespace Heart
{
    class VulkanGraphicsPipeline : public GraphicsPipeline
    {
    public:
        VulkanGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo);
        ~VulkanGraphicsPipeline() override;

    private:

    };
}