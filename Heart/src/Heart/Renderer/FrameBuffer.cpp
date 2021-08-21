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
        if (m_GraphicsPipelines.find(name) != m_GraphicsPipelines.end())
        {
            HE_ENGINE_LOG_ERROR("Cannot register pipeline, name already exists: {0}", name);
            HE_ENGINE_ASSERT(false);
        }
        Ref<GraphicsPipeline> newPipeline = InternalInitializeGraphicsPipeline(createInfo);
        m_GraphicsPipelines[name] = newPipeline;
        return newPipeline;
    }

    Ref<GraphicsPipeline> FrameBuffer::LoadPipeline(const std::string& name)
    {
        if (m_GraphicsPipelines.find(name) == m_GraphicsPipelines.end())
        {
            HE_ENGINE_LOG_ERROR("Pipeline not registered: {0}", name);
            HE_ENGINE_ASSERT(false);
        }
        return m_GraphicsPipelines[name];
    }
}