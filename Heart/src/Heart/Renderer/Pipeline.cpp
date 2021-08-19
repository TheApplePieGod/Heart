#include "htpch.h"
#include "Pipeline.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanGraphicsPipeline.h"

namespace Heart
{
    Ref<GraphicsPipeline> GraphicsPipeline::Create(const GraphicsPipelineCreateInfo& createInfo)
    {
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create GraphicsPipeline: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanGraphicsPipeline>(createInfo); }
        }
    }
    
    Ref<ComputePipeline> ComputePipeline::Create(const ComputePipelineCreateInfo& createInfo)
    {
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create ComputePipeline: selected ApiType is not supported"); return nullptr; }
            //case RenderApi::Type::Vulkan:
            //{ return CreateRef<VulkanComputePipeline>(createInfo); }
        }
    }

    Ref<Pipeline> PipelineRegistry::RegisterGraphicsPipeline(const std::string& name, const GraphicsPipelineCreateInfo& createInfo)
    {
        HE_ENGINE_ASSERT(m_Pipelines.find(name) == m_Pipelines.end(), "Cannot register graphics pipeline, name already exists: {1}", name);
        Ref<Pipeline> newPipeline = GraphicsPipeline::Create(createInfo);
        m_Pipelines[name] = newPipeline;
        return newPipeline;
    }
    
    Ref<Pipeline> PipelineRegistry::RegisterComputePipeline(const std::string& name, const ComputePipelineCreateInfo& createInfo)
    {
        HE_ENGINE_ASSERT(m_Pipelines.find(name) == m_Pipelines.end(), "Cannot register compute pipeline, name already exists: {1}", name);
        Ref<Pipeline> newPipeline = ComputePipeline::Create(createInfo);
        m_Pipelines[name] = newPipeline;
        return newPipeline;
    }

    Ref<Pipeline> PipelineRegistry::LoadPipeline(const std::string& name)
    {
        HE_ENGINE_ASSERT(m_Pipelines.find(name) != m_Pipelines.end(), "Pipeline not registered: {1}", name);
        return m_Pipelines[name];
    }
}