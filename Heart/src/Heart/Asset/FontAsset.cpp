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
        
        m_FontGeometry = msdf_atlas::FontGeometry(&m_Glyphs);
        
        // Load ASCII glyph set
        m_FontGeometry.loadCharset(font, 1.0, msdf_atlas::Charset::ASCII);
        
        // Apply MSDF edge coloring
        const double maxCornerAngle = 3.0;
        for (auto& glyph : m_Glyphs)
            glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);
        
        // Compute atlas layout
        msdf_atlas::TightAtlasPacker packer;
        packer.setDimensionsConstraint(msdf_atlas::TightAtlasPacker::DimensionsConstraint::SQUARE);
        packer.setMinimumScale(24.0);
        packer.setPixelRange(2.0);
        packer.setMiterLimit(1.0);
        packer.pack(m_Glyphs.data(), m_Glyphs.size());
        
        // Get final atlas dimensions
        int width = 0, height = 0;
        packer.getDimensions(width, height);
        m_GlyphScale = (float)packer.getScale();
        m_PixelRange = (float)packer.getPixelRange();
        
        // Generate output atlas
        msdf_atlas::ImmediateAtlasGenerator<
            float,
            4,
            &msdf_atlas::mtsdfGenerator,
            msdf_atlas::BitmapAtlasStorage<byte, 4>
        > generator(width, height);
        msdf_atlas::GeneratorAttributes attributes;
        generator.setAttributes(attributes);
        generator.setThreadCount(4);
        generator.generate(m_Glyphs.data(), m_Glyphs.size());
        
        // Parse pixel data
        std::vector<byte> pixels(4 * width * height);
        msdfgen::BitmapConstRef<msdfgen::byte, 4> bitmapRef = generator.atlasStorage();
        for (int y = 0; y < height; y++)
        {
            memcpy(
               &pixels[4 * width * y],
               bitmapRef(0, height - y - 1),
               4 * width
            );
        }
            
        // Create atlas texture
        Flourish::TextureCreateInfo createInfo = {
            static_cast<u32>(width),
            static_cast<u32>(height),
            Flourish::ColorFormat::RGBA8_UNORM,
            Flourish::TextureUsageType::Readonly,
            Flourish::TextureWritability::Once,
            1, 1,
            Flourish::TextureSamplerState(),
            pixels.data(),
            static_cast<u32>(width * height * 4),
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
        m_FontGeometry = msdf_atlas::FontGeometry();
        m_Data = nullptr;
        m_Valid = false;
    }
}
