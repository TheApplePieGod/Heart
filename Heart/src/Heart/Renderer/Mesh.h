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
        };

    public:
        Mesh(const std::vector<Vertex>& vertices, const std::vector<u32>& indices, u32 materialIndex);
        Mesh() = default;

        const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
        Buffer* GetVertexBuffer() { return m_VertexBuffer.get(); }
        Buffer* GetIndexBuffer() { return m_IndexBuffer.get(); }
        u32 GetMaterialIndex() const { return m_MaterialIndex; }

    public:
        static const BufferLayout& GetVertexLayout() { return s_VertexLayout; }
        inline static const std::vector<Vertex> DefaultCubeVertices = {
            { { -0.5f, -0.5f, -0.5f }, { 0.f, 1.f } }, // -Z
            { { 0.5f, -0.5f, -0.5f }, { 1.f, 1.f } },
            { { 0.5f, 0.5f, -0.5f }, { 1.f, 0.f } },
            { { -0.5f, 0.5f, -0.5f }, { 0.f, 0.f } },

            { { -0.5f, -0.5f, 0.5f }, { 0.f, 1.f } }, // +Z
            { { 0.5f, -0.5f, 0.5f }, { 1.f, 1.f } },
            { { 0.5f, 0.5f, 0.5f }, { 1.f, 0.f } },
            { { -0.5f, 0.5f, 0.5f }, { 0.f, 0.f } },

            { { 0.5f, -0.5f, -0.5f }, { 0.f, 1.f } }, // +X
            { { 0.5f, -0.5f, 0.5f }, { 1.f, 1.f } },
            { { 0.5f, 0.5f, 0.5f }, { 1.f, 0.f } },
            { { 0.5f, 0.5f, -0.5f }, { 0.f, 0.f } },

            { { -0.5f, -0.5f, -0.5f }, { 0.f, 1.f } }, // -X
            { { -0.5f, -0.5f, 0.5f }, { 1.f, 1.f } },
            { { -0.5f, 0.5f, 0.5f }, { 1.f, 0.f } },
            { { -0.5f, 0.5f, -0.5f }, { 0.f, 0.f } },

            { { -0.5f, 0.5f, -0.5f }, { 0.f, 1.f } }, // +Y
            { { -0.5f, 0.5f, 0.5f }, { 0.f, 0.f } },
            { { 0.5f, 0.5f, 0.5f }, { 1.f, 0.f } },
            { { 0.5f, 0.5f, -0.5f }, { 1.f, 1.f } },

            { { -0.5f, -0.5f, -0.5f }, { 0.f, 1.f } }, // -Y
            { { -0.5f, -0.5f, 0.5f }, { 0.f, 0.f } },
            { { 0.5f, -0.5f, 0.5f }, { 1.f, 0.f } },
            { { 0.5f, -0.5f, -0.5f }, { 1.f, 1.f } }
        };

    private:
        inline static const BufferLayout s_VertexLayout = {
            { BufferDataType::Float3 },
            { BufferDataType::Float2 },
            { BufferDataType::Float3 },
            { BufferDataType::Float4 }
        };

    private:
        std::vector<Vertex> m_Vertices;
        std::vector<u32> m_Indices;
        u32 m_MaterialIndex;
        Ref<Buffer> m_VertexBuffer;
        Ref<Buffer> m_IndexBuffer;
    };
}