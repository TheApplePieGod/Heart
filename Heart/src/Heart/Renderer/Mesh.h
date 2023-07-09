#pragma once

#include "glm/vec4.hpp"
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include "Heart/Container/HVector.hpp"
#include "Flourish/Api/Buffer.h"

namespace Flourish
{
    class AccelerationStructure;
};

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
        const Flourish::Buffer* GetVertexBuffer() const { return m_VertexBuffer.get(); }
        const Flourish::Buffer* GetIndexBuffer() const { return m_IndexBuffer.get(); }
        const Flourish::AccelerationStructure* GetAccelStructure() const { return m_AccelStructure.get(); }
        u32 GetMaterialIndex() const { return m_MaterialIndex; }
        const AABB& GetBoundingBox() const { return m_BoundingBox; }
        glm::vec4 GetBoundingSphere() const { return m_BoundingSphere; }

    public:
        static const Flourish::BufferLayout& GetVertexLayout() { return s_VertexLayout; }
        static const Flourish::BufferLayout& GetIndexLayout() { return s_IndexLayout; }

    private:
        inline static const Flourish::BufferLayout s_VertexLayout = {
            { Flourish::BufferDataType::Float3 },
            { Flourish::BufferDataType::Float2 },
            { Flourish::BufferDataType::Float3 },
            { Flourish::BufferDataType::Float4 }
        };
        inline static const Flourish::BufferLayout s_IndexLayout = {
            { Flourish::BufferDataType::UInt }
        };

    private:
        void CalculateBounds();

    private:
        u32 m_BufferReadyCount = 0;
        HVector<Vertex> m_Vertices;
        HVector<u32> m_Indices;
        u32 m_MaterialIndex;
        Ref<Flourish::Buffer> m_VertexBuffer;
        Ref<Flourish::Buffer> m_IndexBuffer;
        Ref<Flourish::AccelerationStructure> m_AccelStructure = nullptr;
        AABB m_BoundingBox;
        glm::vec4 m_BoundingSphere; // xyz: center, w: radius
    };
}
