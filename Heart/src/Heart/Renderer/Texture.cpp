#include "htpch.h"
#include "Texture.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanTexture.h"

namespace Heart
{
    Ref<Texture> Texture::Create(const std::string& path)
    {
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create texture: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanTexture>(path); }
        }
    }

    Ref<Texture> TextureRegistry::RegisterTexture(const std::string& name, const std::string& path)
    {
        if (m_Textures.find(name) != m_Textures.end())
        {
            HE_ENGINE_LOG_ERROR("Cannot register texture, name already exists: {0}", name);
            HE_ENGINE_ASSERT(false);
        }

        HE_ENGINE_LOG_TRACE("Registering texture '{0}' @ '{1}'", name, path);

        Ref<Texture> newTexture = Texture::Create(path);
        m_Textures[name] = newTexture;
        return newTexture;
    }
    
    Ref<Texture> TextureRegistry::LoadTexture(const std::string& name)
    {
        if (m_Textures.find(name) == m_Textures.end())
        {
            HE_ENGINE_LOG_ERROR("Texture not registered: {0}", name);
            HE_ENGINE_ASSERT(false);
        }
        return m_Textures[name];
    }
}