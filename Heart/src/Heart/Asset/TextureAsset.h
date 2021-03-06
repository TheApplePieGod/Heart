#pragma once

#include "Heart/Asset/Asset.h"

namespace Heart
{
    class Texture;
    class TextureAsset : public Asset
    {
    public:
        /**
         * @brief Default constructor.
         * 
         * @param path The path of the asset relative to the project directory.
         * @param absolutePath The absolute filesystem path of the asset.
         */
        TextureAsset(const std::string& path, const std::string& absolutePath)
            : Asset(path, absolutePath)
        { m_Type = Type::Texture; }

        void Load() override;
        void Unload() override;

        /*! @brief Get a pointer to the texture stored in this asset. */
        Texture* GetTexture() { return m_Texture.get(); }

    private:
        const int m_DesiredChannelCount = 4; // all images will load as RGBA
        Ref<Texture> m_Texture;
    };
}