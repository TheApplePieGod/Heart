#include "hepch.h"
#include "Mesh.h"

namespace Heart
{
    Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<u32>& indices, u32 materialIndex)
        : m_Vertices(vertices), m_Indices(indices), m_MaterialIndex(materialIndex)
    {
        m_VertexBuffer = Buffer::Create(Buffer::Type::Vertex, BufferUsageType::Static, s_VertexLayout, static_cast<u32>(vertices.size()), (void*)vertices.data());
        m_IndexBuffer = Buffer::CreateIndexBuffer(BufferUsageType::Static, static_cast<u32>(indices.size()), (void*)indices.data());
    }
}