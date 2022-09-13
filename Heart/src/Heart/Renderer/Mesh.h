#pragma once

#include "glm/vec4.hpp"
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include "Heart/Renderer/Buffer.h"

namespace Heart
{
    struct AABB
    {
        glm::vec3 Min = { 0.f, 0.f, 0.f };
        glm::vec3 Max = { 0.f, 0.f, 0.f };
    };

    class Mesh
    {
    public:
        struct Vertex
        {
            glm::vec3 Position = { 0.f, 0.f, 0.f };
            glm::vec2 UV = { 0.f, 0.f };
            glm::vec3 Normal = { 0.f, 0.f, 0.f };
            glm::vec4 Tangent = { 0.f, 0.f, 0.f, 1.f };
        };

    public:
        Mesh(const HVector<Vertex>& vertices, const HVector<u32>& indices, u32 materialIndex);
        Mesh() = default;

        const HVector<Vertex>& GetVertices() const { return m_Vertices; }
        Buffer* GetVertexBuffer() { return m_VertexBuffer.get(); }
        Buffer* GetIndexBuffer() { return m_IndexBuffer.get(); }
        u32 GetMaterialIndex() const { return m_MaterialIndex; }
        const AABB& GetBoundingBox() const { return m_BoundingBox; }
        glm::vec4 GetBoundingSphere() const { return m_BoundingSphere; }

    public:
        static const BufferLayout& GetVertexLayout() { return s_VertexLayout; }

    private:
        inline static const BufferLayout s_VertexLayout = {
            { BufferDataType::Float3 },
            { BufferDataType::Float2 },
            { BufferDataType::Float3 },
            { BufferDataType::Float4 }
        };

    private:
        void CalculateBounds();

    private:
        HVector<Vertex> m_Vertices;
        HVector<u32> m_Indices;
        u32 m_MaterialIndex;
        Ref<Buffer> m_VertexBuffer;
        Ref<Buffer> m_IndexBuffer;
        AABB m_BoundingBox;
        glm::vec4 m_BoundingSphere; // xyz: center, w: radius
    };
}