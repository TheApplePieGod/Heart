#include "hepch.h"
#include "Pipeline.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Heart/Platform/Vulkan/VulkanComputePipeline.h"
#include "Heart/Platform/OpenGL/OpenGLComputePipeline.h"

namespace Heart
{
    Ref<ComputePipeline> ComputePipeline::Create(const ComputePipelineCreateInfo& createInfo)
    {
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create ComputePipeline: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanComputePipeline>(createInfo); }
            case RenderApi::Type::OpenGL:
            { return CreateRef<OpenGLComputePipeline>(createInfo); }
        }
        
        return nullptr;
    }

    void Pipeline::SortReflectionData()
    {
        std::sort(m_ProgramReflectionData.begin(), m_ProgramReflectionData.end(), [](const ReflectionDataElement& a, const ReflectionDataElement& b)
        {
            return a.BindingIndex < b.BindingIndex;
        });
    }

    void GraphicsPipeline::ConsolidateReflectionData()
    {
        auto vertShader = AssetManager::RetrieveAsset<ShaderAsset>(m_Info.VertexShaderAsset)->GetShader();
        auto fragShader = AssetManager::RetrieveAsset<ShaderAsset>(m_Info.FragmentShaderAsset)->GetShader();

        m_ProgramReflectionData.clear();
        m_ProgramReflectionData.insert(m_ProgramReflectionData.end(), vertShader->GetReflectionData().begin(), vertShader->GetReflectionData().end());

        for (auto& fragData : fragShader->GetReflectionData())
        {
            bool add = true;
            for (auto& vertData : m_ProgramReflectionData)
            {
                if (fragData.BindingIndex == vertData.BindingIndex)
                {
                    HE_ENGINE_ASSERT(fragData.UniqueId != vertData.UniqueId, "Binding index must be unique for all shader resources");
                    vertData.AccessType = ShaderResourceAccessType::Both;
                    add = false;
                    break;
                }
            }
            if (add)
                m_ProgramReflectionData.push_back(fragData);
        }

        SortReflectionData();
    }

    void ComputePipeline::ConsolidateReflectionData()
    {
        auto compShader = AssetManager::RetrieveAsset<ShaderAsset>(m_Info.ComputeShaderAsset)->GetShader();

        m_ProgramReflectionData.clear();
        m_ProgramReflectionData.insert(m_ProgramReflectionData.end(), compShader->GetReflectionData().begin(), compShader->GetReflectionData().end());

        SortReflectionData();
    }
}