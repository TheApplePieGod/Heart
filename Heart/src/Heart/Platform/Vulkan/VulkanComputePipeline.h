#pragma once

#include "Heart/Renderer/Pipeline.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"
#include "Heart/Platform/Vulkan/VulkanDescriptorSet.h"

namespace Heart
{
    class VulkanComputePipeline : public ComputePipeline
    {
    public:
        VulkanComputePipeline(const ComputePipelineCreateInfo& createInfo);
        ~VulkanComputePipeline() override;

        void Bind() override;

        void BindShaderBufferResource(u32 bindingIndex, u32 elementOffset, u32 elementCount, Buffer* buffer)  override;
        void BindShaderTextureResource(u32 bindingIndex, Texture* texture) override;
        void BindShaderTextureLayerResource(u32 bindingIndex, Texture* texture, u32 layerIndex, u32 mipLevel) override;
        void FlushBindings() override;

        void Submit();

        inline VkCommandBuffer GetCommandBuffer() { UpdateFrameIndex(); return m_CommandBuffers[m_InFlightFrameIndex]; }
        inline VkPipeline GetPipeline() const { return m_Pipeline; }
        inline VkPipelineLayout GetLayout() const { return m_PipelineLayout; }
        inline VulkanDescriptorSet& GetVulkanDescriptorSet() { return m_DescriptorSet; }

    private:
        void UpdateFrameIndex();
        void BindShaderResource(u32 bindingIndex, ShaderResourceType resourceType, void* resource, bool useOffset, u32 offset, u32 size); // buffer offset: bytes, image offset: layerIndex, buffer size: bytes

    private:
        VulkanDescriptorSet m_DescriptorSet; // for now, we only utilize one descriptor set in each shader

        VkPipelineLayout m_PipelineLayout;
        VkPipeline m_Pipeline;
        std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_CommandBuffers{};

        u64 m_LastUpdateFrame = 0;
        u32 m_InFlightFrameIndex = 0;
        bool m_FlushedThisFrame = false;
        bool m_BoundThisFrame = false;
        bool m_SubmittedThisFrame = false;
    };
}