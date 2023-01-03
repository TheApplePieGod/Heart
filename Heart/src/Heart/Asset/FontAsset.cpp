#include "hepch.h"
#include "FontAsset.h"

#include "Heart/Asset/AssetManager.h"

namespace Heart
{
    void FontAsset::Load(bool async)
    {
        HE_PROFILE_FUNCTION();
        
        if (m_Loaded || m_Loading) return;
        m_Loading = true;
        
        auto failedFunc = [&]()
        {
            HE_ENGINE_LOG_ERROR("Failed to load font at path {0}", m_AbsolutePath.Data());
            m_Loaded = true;
            m_Loading = false;
        };
        
        // Init freetype
        msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
        if (!ft)
        {
            failedFunc();
            return;
        }
        
        // Load font file
        msdfgen::FontHandle* font = msdfgen::loadFont(ft, m_AbsolutePath.Data());
        if (!font)
        {
            failedFunc();
            return;
        }
        
        std::vector<msdf_atlas::GlyphGeometry> glyphs;
        msdf_atlas::FontGeometry fontGeometry(&glyphs);
        
        // Load ASCII glyph set
        fontGeometry.loadCharset(font, 1.0, msdf_atlas::Charset::ASCII);
        
        // Apply MSDF edge coloring
        const double maxCornerAngle = 3.0;
        for (auto& glyph : glyphs)
            glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);
        
        // Compute atlas layout
        msdf_atlas::TightAtlasPacker packer;
        packer.setDimensionsConstraint(msdf_atlas::TightAtlasPacker::DimensionsConstraint::SQUARE);
        packer.setMinimumScale(24.0);
        packer.setPixelRange(2.0);
        packer.setMiterLimit(1.0);
        packer.pack(glyphs.data(), glyphs.size());
        
        // Get final atlas dimensions
        int width = 0, height = 0;
        packer.getDimensions(width, height);
        
        // Generate output atlas
        msdf_atlas::ImmediateAtlasGenerator<
            float,
            3,
            &msdf_atlas::msdfGenerator,
            msdf_atlas::BitmapAtlasStorage<byte, 3>
        > generator(width, height);
        msdf_atlas::GeneratorAttributes attributes;
        generator.setAttributes(attributes);
        generator.setThreadCount(4);
        generator.generate(glyphs.data(), glyphs.size());
        
        // Parse pixel data
        std::vector<byte> pixels(3 * width * height);
        msdfgen::BitmapConstRef<msdfgen::byte, 3> bitmapRef = generator.atlasStorage();
        for (int y = 0; y < height; y++)
        {
            memcpy(
               &pixels[3 * width * y],
               bitmapRef(0, height - y - 1),
               3 * width
            );
        }
            
        // Create atlas texture
        Flourish::TextureCreateInfo createInfo = {
            static_cast<u32>(width),
            static_cast<u32>(height),
            Flourish::ColorFormat::RGB8_UNORM,
            Flourish::TextureUsageType::Readonly,
            Flourish::TextureWritability::Once,
            1, 1,
            Flourish::TextureSamplerState(),
            pixels.data(),
            static_cast<u32>(width * height * 3),
            async,
            [this]()
            {
                if (!AssetManager::IsInitialized()) return;

                m_Loaded = true;
                m_Loading = false;
                m_Valid = true;
            }
        };
        m_AtlasTexture = Flourish::Texture::Create(createInfo);
            
        // Cleanup
        msdfgen::destroyFont(font);
        msdfgen::deinitializeFreetype(ft);
        
        // Map glyphs
        for (auto& glyph : glyphs)
            m_Glyphs[glyph.getCodepoint()] = glyph;
        
        m_Loaded = true;
        m_Loading = false;
        m_Valid = true;
    }

    void FontAsset::Unload()
    {
        if (!m_Loaded) return;
        m_Loaded = false;

        m_AtlasTexture.reset();
        m_Glyphs.clear();
        m_Data = nullptr;
        m_Valid = false;
    }
}
