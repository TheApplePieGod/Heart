#include "htpch.h"
#include "Pipeline.h"

#include <algorithm>

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

    void GraphicsPipeline::ConsolidateReflectionData()
    {
        m_ProgramReflectionData.clear();

        m_ProgramReflectionData.insert(m_ProgramReflectionData.end(), m_Info.VertexShader->GetReflectionData().begin(), m_Info.VertexShader->GetReflectionData().end());

        for (auto& fragData : m_Info.FragmentShader->GetReflectionData())
        {
            for (auto& vertData : m_ProgramReflectionData)
            {
                if (fragData.BindingIndex == vertData.BindingIndex)
                {
                    HE_ENGINE_ASSERT(fragData.UniqueId != vertData.UniqueId, "Binding index must be unique for all shader resources");
                    vertData.AccessType = ShaderResourceAccessType::Both;
                }
            }
            m_ProgramReflectionData.push_back(fragData);
        }

        std::sort(m_ProgramReflectionData.begin(), m_ProgramReflectionData.end(), [](const ReflectionDataElement& a, const ReflectionDataElement& b)
        {
            return a.BindingIndex < b.BindingIndex;
        });
    }
}