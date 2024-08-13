#pragma once

#include "Heart/Asset/Asset.h"
#include "Flourish/Api/Texture.h"

namespace Heart
{
    class TextureAsset : public Asset
    {
    public:
        /**
         * @brief Default constructor.
         * 
         * @param path The path of the asset relative to the project directory.
         * @param absolutePath The absolute filesystem path of the asset.
         */
        TextureAsset(const HString8& path, const HString8& absolutePath)
            : Asset(path, absolutePath)
        { m_Type = Type::Texture; }

        /*! @brief Get a reference to the texture stored in this asset. */
        inline const Ref<Flourish::Texture>& GetTexture() { return m_Texture; }

    protected:
        void LoadInternal() override;
        void UnloadInternal() override;
        bool ShouldUnload() override;

    private:
        void* LoadImage(int& outWidth, int& outHeight, int& outChannels, bool floatComponents);
        void* LoadTiff(int& outWidth, int& outHeight, int& outChannels);

    private:
        const int m_DesiredChannelCount = 4; // all images will load as RGBA
        Ref<Flourish::Texture> m_Texture;
    };
}
