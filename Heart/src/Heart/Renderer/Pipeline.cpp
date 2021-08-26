#include "htpch.h"
#include "Pipeline.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanGraphicsPipeline.h"

namespace Heart
{
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
}