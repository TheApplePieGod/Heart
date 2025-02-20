#include "hepch.h"
#include "FontAsset.h"

#include "Heart/Asset/AssetManager.h"
#include "Heart/Util/FilesystemUtils.h"

namespace Heart
{
    void FontAsset::LoadInternal()
    {
        HE_PROFILE_FUNCTION();

        auto failedFunc = [&]()
        {
            HE_ENGINE_LOG_ERROR("Failed to load font at path {0}", m_AbsolutePath.Data());
        };
        
        // Init freetype
        msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
        if (!ft)
        {
            failedFunc();
            return;
        }

        // Read font data
        u32 fileLength;
        unsigned char* data = FilesystemUtils::ReadFile(m_AbsolutePath, fileLength);
        if (!data)
        {
            failedFunc();
            return;
        }
        
        // Load font file
        msdfgen::FontHandle* font = msdfgen::loadFontData(ft, data, fileLength);
        if (!font)
        {
            delete[] data;
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
        packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
        packer.setMinimumScale(24.0);
        packer.setPixelRange(2.0);
        packer.setMiterLimit(1.0);
        packer.pack(m_Glyphs.data(), m_Glyphs.size());
        
        // Get final atlas dimensions
        int width = 0, height = 0;
        packer.getDimensions(width, height);
        m_GlyphScale = (float)packer.getScale();
        m_PixelRange = (packer.getPixelRange().upper - packer.getPixelRange().lower);
        
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
            Flourish::TextureUsageFlags::Readonly,
            1, 1,
            Flourish::TextureSamplerState(),
            pixels.data(),
            static_cast<u32>(width * height * 4)
        };
        m_AtlasTexture = Flourish::Texture::Create(createInfo);
            
        // Cleanup
        msdfgen::destroyFont(font);
        msdfgen::deinitializeFreetype(ft);
        
        delete[] data;
        m_Valid = true;
    }

    void FontAsset::UnloadInternal()
    {
        m_AtlasTexture.reset();
        m_Glyphs.clear();
        m_FontGeometry = msdf_atlas::FontGeometry();
        m_Data = nullptr;
    }

    bool FontAsset::ShouldUnload()
    {
        // This is the only remaining reference
        return m_AtlasTexture.use_count() == 1;
    }
}
