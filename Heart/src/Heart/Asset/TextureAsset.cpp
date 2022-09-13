#include "hepch.h"
#include "TextureAsset.h"

#include "Heart/Asset/AssetManager.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Renderer/Texture.h"
#include "stb_image/stb_image.h"

namespace Heart
{
    void TextureAsset::Load(bool async)
    {
        HE_PROFILE_FUNCTION();
        
        if (m_Loaded || m_Loading) return;
        m_Loading = true;

        bool floatComponents = false;
        if (m_Extension == ".hdr") // environment map: use float components and flip on load
        {
            floatComponents = true;
            stbi_set_flip_vertically_on_load_thread(true);
        }
        else
            stbi_set_flip_vertically_on_load_thread(false);

        void* pixels = nullptr;
        int width, height, channels;
        if (floatComponents)
            pixels = stbi_loadf(m_AbsolutePath.Data(), &width, &height, &channels, m_DesiredChannelCount);
        else
            pixels = stbi_load(m_AbsolutePath.Data(), &width, &height, &channels, m_DesiredChannelCount);
        if (pixels == nullptr)
        {
            HE_ENGINE_LOG_ERROR("Failed to load texture at path {0}", m_AbsolutePath.Data());
            m_Loaded = true;
            m_Loading = false;
            return;
        }

        TextureCreateInfo createInfo = {
            static_cast<u32>(width), static_cast<u32>(height), static_cast<u32>(m_DesiredChannelCount),
            floatComponents ? BufferDataType::Float : BufferDataType::UInt8,
            BufferUsageType::Static,
            1, 0
        };

        auto finalizeFn = [this, createInfo, pixels, floatComponents]()
        {
            if (!AssetManager::IsInitialized()) return;
            m_Texture = Texture::Create(createInfo, pixels);
            if (floatComponents)
                delete[] (float*)pixels;
            else
                delete[] (unsigned char*)pixels;

            m_Loaded = true;
            m_Loading = false;
            m_Valid = true;
        };

        if (async)
            Renderer::PushJobQueue(finalizeFn);
        else
            finalizeFn();
    }

    void TextureAsset::Unload()
    {
        if (!m_Loaded) return;

        m_Texture.reset();
        //delete[] m_Data;
        m_Data = nullptr;
        m_Valid = false;
        m_Loaded = false;
    }
}