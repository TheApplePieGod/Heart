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
        FontAsset(const HString8& path, const HString8& absolutePath)
            : Asset(path, absolutePath)
        { m_Type = Type::Font; }

        inline Flourish::Texture* GetAtlasTexture() { return m_AtlasTexture.get(); }
        inline const auto& GetFontGeometry() const { return m_FontGeometry; }
        inline float GetPixelRange() const { return m_PixelRange; }
        inline float GetGlyphScale() const { return m_GlyphScale; }

    protected:
        void LoadInternal() override;
        void UnloadInternal() override;
        bool ShouldUnload() override;
        
    private:
        Ref<Flourish::Texture> m_AtlasTexture;
        msdf_atlas::FontGeometry m_FontGeometry;
        std::vector<msdf_atlas::GlyphGeometry> m_Glyphs;
        float m_PixelRange;
        float m_GlyphScale;
    };
}
