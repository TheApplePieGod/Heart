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
        TextureAsset(const HStringView8& path, const HStringView8& absolutePath)
            : Asset(path, absolutePath)
        { m_Type = Type::Texture; }

        void Load(bool async = false) override;
        void Unload() override;

        /*! @brief Get a pointer to the texture stored in this asset. */
        inline Texture* GetTexture() { return m_Texture.get(); }

    private:
        const int m_DesiredChannelCount = 4; // all images will load as RGBA
        Ref<Texture> m_Texture;
    };
}