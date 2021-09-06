#pragma once

#include "Heart/Renderer/Pipeline.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"
#include "Heart/Platform/Vulkan/VulkanDescriptorSet.h"

namespace Heart
{
    class VulkanGraphicsPipeline : public GraphicsPipeline
    {
    public:
        VulkanGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo, VkRenderPass renderPass, VkSampleCountFlagBits sampleCount, u32 viewportWidth, u32 viewportHeight);
        ~VulkanGraphicsPipeline() override;

        inline VkPipeline GetPipeline() const { return m_Pipeline; }
        inline VkPipelineLayout GetLayout() const { return m_PipelineLayout; }
        inline VulkanDescriptorSet& GetVulkanDescriptorSet() { return m_DescriptorSet; }

    private:
        VulkanDescriptorSet m_DescriptorSet; // for now, we only utilize one descriptor set in each shader

        VkPipelineLayout m_PipelineLayout;
        VkPipeline m_Pipeline;
    };
}