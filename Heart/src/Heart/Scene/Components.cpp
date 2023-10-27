#include "hepch.h"
#include "Components.h"

#include "Heart/Renderer/Mesh.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/FontAsset.h"
#include "Heart/Asset/MeshAsset.h"
#include "Flourish/Api/Buffer.h"

namespace Heart
{
    void TextComponent::ClearRenderData()
    {
        ComputedMesh = Mesh();
    }

    void TextComponent::RecomputeRenderData()
    {
        if (Text.Count() == 0) return;
        if (Recomputing) return;
        Recomputing = true;
        
        auto fontAsset = AssetManager::RetrieveAsset<FontAsset>(Font);
        if (!fontAsset || !fontAsset->Load(false)->IsValid())
            return;

        HVector<Mesh::Vertex> vertices;
        HVector<u32> indices;
        vertices.Reserve(Text.Count() * 4);
        indices.Reserve(Text.Count() * 6);
        
        // Facing forward +z direction
        Mesh::Vertex vertex;
        vertex.Normal = { 0.f, 0.f, 1.f };
        vertex.Tangent = { -1.f, 0.f, 0.f, 1.0f };
        u32 vertexOffset = 0;
        glm::vec3 localPos = { 0.f, 0.f, 0.f };
        float lineHeight = LineHeight > 0 ? LineHeight : (float)fontAsset->GetFontGeometry().getMetrics().lineHeight;
        lineHeight *= FontSize;
        for (u32 i = 0; i < Text.Count(); i++)
        {
            char c = Text.GetUTF8(i);
            auto glyph = fontAsset->GetFontGeometry().getGlyph((u32)c);
            
            // Advance glyph
            float scale = FontSize;
            if (c == '\n')
            {
                localPos.x = 0.f;
                localPos.y -= lineHeight;
                
                continue;
            }
            
            if (!glyph) continue;
            
            if (i != 0 && Text.GetUTF8(i - 1) != '\n')
            {
                double advance = 0.0;
                fontAsset->GetFontGeometry().getAdvance(advance, (u32)Text.GetUTF8(i - 1), (u32)c);
                localPos.x -= (float)advance * scale;
            }
            
            double ql, qb, qr, qt;
            double pl, pb, pr, pt;
            glyph->getQuadAtlasBounds(ql, qb, qr, qt);
            glyph->getQuadPlaneBounds(pl, pb, pr, pt);
            glm::vec2 atlasDim = { fontAsset->GetAtlasTexture()->GetWidth(), fontAsset->GetAtlasTexture()->GetHeight() };
            glm::vec2 uvMin = { (float)qr / atlasDim.x, 1.0f - ((float)qt / atlasDim.y) };
            glm::vec2 uvMax = { (float)ql / atlasDim.x, 1.0f - ((float)qb / atlasDim.y) };
            glm::vec2 posMin = { (float)-pr * scale, (float)pt * scale };
            glm::vec2 posMax = { (float)-pl * scale, (float)pb * scale };
            
            // Vertices
            vertex.Position = localPos + glm::vec3(posMin.x, posMin.y, 0.f);
            vertex.UV = uvMin;
            vertices.AddInPlace(vertex); // TL
            vertex.Position = localPos + glm::vec3(posMax.x, posMin.y, 0.f);
            vertex.UV = { uvMax.x, uvMin.y };
            vertices.AddInPlace(vertex); // TR
            vertex.Position = localPos + glm::vec3(posMin.x, posMax.y, 0.f);
            vertex.UV = { uvMin.x, uvMax.y };
            vertices.AddInPlace(vertex); // BL
            vertex.Position = localPos + glm::vec3(posMax.x, posMax.y, 0.f);
            vertex.UV = uvMax;
            vertices.AddInPlace(vertex); // BR
            
            // Triangle 1 indices
            indices.Add(vertexOffset);
            indices.Add(vertexOffset + 2);
            indices.Add(vertexOffset + 1);
            
            // Triangle 2 indices
            indices.Add(vertexOffset + 1);
            indices.Add(vertexOffset + 2);
            indices.Add(vertexOffset + 3);
            
            vertexOffset += 4;
        }

        ComputedMesh = Mesh(vertices, indices, 0);

        Recomputing = false;
    }
}
