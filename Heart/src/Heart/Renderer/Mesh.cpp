#include "hepch.h"
#include "Mesh.h"

#include "glm/glm.hpp"

namespace Heart
{
    Mesh::Mesh(const HVector<Vertex>& vertices, const HVector<u32>& indices, u32 materialIndex)
        : m_Vertices(vertices), m_Indices(indices), m_MaterialIndex(materialIndex)
    {
        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Type = Flourish::BufferType::Vertex;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Static;
        bufCreateInfo.Layout = s_VertexLayout;
        bufCreateInfo.ElementCount = vertices.Count();
        bufCreateInfo.InitialData = vertices.Data();
        bufCreateInfo.InitialDataSize = sizeof(Vertex) * vertices.Count();
        m_VertexBuffer = Flourish::Buffer::Create(bufCreateInfo);

        bufCreateInfo.Type = Flourish::BufferType::Index;
        bufCreateInfo.Layout = s_IndexLayout;
        bufCreateInfo.ElementCount = indices.Count();
        bufCreateInfo.InitialData = indices.Data();
        bufCreateInfo.InitialDataSize = sizeof(u32) * indices.Count();
        m_IndexBuffer = Flourish::Buffer::Create(bufCreateInfo);
        
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