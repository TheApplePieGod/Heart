#pragma once

#include "glm/vec4.hpp"
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include "Heart/Renderer/Buffer.h"

namespace Heart
{
    class Mesh
    {
    public:
        struct Vertex
        {
            glm::vec3 Position = { 0.f, 0.f, 0.f };
            glm::vec2 UV = { 0.f, 0.f };
            glm::vec3 Normal = { 0.f, 0.f, 0.f };
            glm::vec4 Tangent = { 0.f, 0.f, 0.f, 1.f };
            u32 MaterialIndex = 0;  
        };

    public:
        Mesh(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<u32>& indices);

        const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
        const std::string& GetName() const { return m_Name; }
        Buffer* GetVertexBuffer() { return m_VertexBuffer.get(); }
        Buffer* GetIndexBuffer() { return m_IndexBuffer.get(); }

    public:
        static const BufferLayout& GetVertexLayout() { return s_VertexLayout; }

    private:
        inline static const BufferLayout s_VertexLayout = {
            { BufferDataType::Float3 },
            { BufferDataType::Float2 },
            { BufferDataType::Float3 },
            { BufferDataType::Float4 },
            { BufferDataType::UInt }
        };

    private:
        std::vector<Vertex> m_Vertices;
        std::vector<u32> m_Indices;
        std::string m_Name;
        Ref<Buffer> m_VertexBuffer;
        Ref<Buffer> m_IndexBuffer;
    };
}