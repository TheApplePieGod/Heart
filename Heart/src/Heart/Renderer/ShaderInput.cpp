#include "htpch.h"
#include "ShaderInput.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanShaderInput.h"

namespace Heart
{
    Ref<ShaderInputSet> ShaderInputSet::Create(std::initializer_list<ShaderInputElement> elements)
    {
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create shader input set: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanShaderInputSet>(elements); }
        }
    }

    Ref<ShaderInputSet> ShaderInputRegistry::RegisterShaderInputSet(const std::string& name, std::initializer_list<ShaderInputElement> elements)
    {
        if (m_ShaderInputSets.find(name) != m_ShaderInputSets.end())
        {
            HE_ENGINE_LOG_ERROR("Cannot register shader input, name already exists: {0}", name);
            HE_ENGINE_ASSERT(false);
        }
        Ref<ShaderInputSet> newSet = ShaderInputSet::Create(elements);
        m_ShaderInputSets[name] = newSet;
        return newSet;
    }
    
    Ref<ShaderInputSet> ShaderInputRegistry::LoadShaderInputSet(const std::string& name)
    {
        if (m_ShaderInputSets.find(name) == m_ShaderInputSets.end())
        {
            HE_ENGINE_LOG_ERROR("Shader input not registered: {0}", name);
            HE_ENGINE_ASSERT(false);
        }
        return m_ShaderInputSets[name];
    }
}