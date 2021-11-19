#include "htpch.h"
#include "Pipeline.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
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
        auto vertShader = AssetManager::RetrieveAsset<ShaderAsset>(m_Info.VertexShaderAsset)->GetShader();
        auto fragShader = AssetManager::RetrieveAsset<ShaderAsset>(m_Info.FragmentShaderAsset)->GetShader();

        m_ProgramReflectionData.clear();
        m_ProgramReflectionData.insert(m_ProgramReflectionData.end(), vertShader->GetReflectionData().begin(), vertShader->GetReflectionData().end());

        for (auto& fragData : fragShader->GetReflectionData())
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