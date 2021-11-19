#pragma once

#include "Heart/Asset/Asset.h"
#include "Heart/Renderer/Texture.h"

namespace Heart
{
    class TextureAsset : public Asset
    {
    public:
        TextureAsset(const std::string& path, const std::string& absolutePath)
            : Asset(path, absolutePath)
        { m_Type = Type::Texture; }

        void Load() override;
        void Unload() override;

        Texture* GetTexture() { return m_Texture.get(); }

    private:
        const int m_DesiredChannelCount = 4; // all images will load as RGBA
        int m_Width, m_Height, m_Channels;
        Ref<Texture> m_Texture;
    };
}