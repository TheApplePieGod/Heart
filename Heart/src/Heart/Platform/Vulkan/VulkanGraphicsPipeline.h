#pragma once

#include "Heart/Renderer/Pipeline.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanGraphicsPipeline : public GraphicsPipeline
    {
    public:
        VulkanGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo, VkRenderPass renderPass, VkSampleCountFlagBits sampleCount, u32 viewportWidth, u32 viewportHeight);
        ~VulkanGraphicsPipeline() override;

        inline VkPipeline GetPipeline() const { return m_Pipeline; }
        inline VkPipelineLayout GetLayout() const { return m_PipelineLayout; }

    private:
        VkPipelineLayout m_PipelineLayout;
        VkPipeline m_Pipeline;
    };
}