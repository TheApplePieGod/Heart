#pragma once

#include "Heart/Asset/Asset.h"
#include "Flourish/Api/Texture.h"

#include "msdf-atlas-gen/msdf-atlas-gen.h"

namespace Heart
{
    class FontAsset : public Asset
    {
    public:
        /**
         * @brief Default constructor.
         * 
         * @param path The path of the asset relative to the project directory.
         * @param absolutePath The absolute filesystem path of the asset.
         */
        FontAsset(const HStringView8& path, const HStringView8& absolutePath)
            : Asset(path, absolutePath)
        { m_Type = Type::Font; }

        void Load(bool async = false) override;
        void Unload() override;

        /*! @brief Get a pointer to the font atlas texture stored in this asset. */
        inline Flourish::Texture* GetAtlasTexture() { return m_AtlasTexture.get(); }
        
        inline const auto& GetGlyphs() const { return m_Glyphs; }

    private:
        Ref<Flourish::Texture> m_AtlasTexture;
        std::unordered_map<u32, msdf_atlas::GlyphGeometry> m_Glyphs;
    };
}
