#include "hepch.h"
#include "Mesh.h"

#include "glm/glm.hpp"

namespace Heart
{
    Mesh::Mesh(const HVector<Vertex>& vertices, const HVector<u32>& indices, u32 materialIndex)
        : m_Vertices(vertices), m_Indices(indices), m_MaterialIndex(materialIndex)
    {
        m_VertexBuffer = Buffer::Create(Buffer::Type::Vertex, BufferUsageType::Static, s_VertexLayout, static_cast<u32>(vertices.GetCount()), (void*)vertices.Data());
        m_IndexBuffer = Buffer::CreateIndexBuffer(BufferUsageType::Static, static_cast<u32>(indices.GetCount()), (void*)indices.Data());
        
        CalculateBounds();
    }

    void Mesh::CalculateBounds()
    {
        AABB bounds;
        for (auto& vertex : m_Vertices)
        {
            if (vertex.Position.x < bounds.Min.x)
                bounds.Min.x = vertex.Position.x;
            else if (vertex.Position.x > bounds.Max.x)
                bounds.Max.x = vertex.Position.x;
                
            if (vertex.Position.y < bounds.Min.y)
                bounds.Min.y = vertex.Position.y;
            else if (vertex.Position.y > bounds.Max.y)
                bounds.Max.y = vertex.Position.y;

            if (vertex.Position.z < bounds.Min.z)
                bounds.Min.z = vertex.Position.z;
            else if (vertex.Position.z > bounds.Max.z)
                bounds.Max.z = vertex.Position.z;
        }
        m_BoundingBox = bounds;

        glm::vec3 center = (bounds.Min + bounds.Max) * 0.5f;
        float radius = glm::length(bounds.Max - center);
        m_BoundingSphere = glm::vec4(center, radius);
    }
}