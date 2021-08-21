#include "htpch.h"
#include "FrameBuffer.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanFrameBuffer.h"

namespace Heart
{
    Ref<FrameBuffer> FrameBuffer::Create(const FrameBufferCreateInfo& createInfo)
    {
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create FrameBuffer: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanFrameBuffer>(createInfo); }
        }
    }

    Ref<GraphicsPipeline> FrameBuffer::RegisterGraphicsPipeline(const std::string& name, const GraphicsPipelineCreateInfo& createInfo)
    {
        HE_ENGINE_ASSERT(m_GraphicsPipelines.find(name) == m_GraphicsPipelines.end(), "Cannot register graphics pipeline, name already exists: {1}", name);
        Ref<GraphicsPipeline> newPipeline = InternalInitializeGraphicsPipeline(createInfo);
        m_GraphicsPipelines[name] = newPipeline;
        return newPipeline;
    }

    Ref<GraphicsPipeline> FrameBuffer::LoadPipeline(const std::string& name)
    {
        HE_ENGINE_ASSERT(m_GraphicsPipelines.find(name) != m_GraphicsPipelines.end(), "Pipeline not registered: {1}", name);
        return m_GraphicsPipelines[name];
    }
}