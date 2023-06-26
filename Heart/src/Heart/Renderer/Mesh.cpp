#include "hepch.h"
#include "Mesh.h"

#include "glm/glm.hpp"
#include "Flourish/Api/Context.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/RenderGraph.h"
#include "Flourish/Api/TransferCommandEncoder.h"
#include "Flourish/Api/ComputeCommandEncoder.h"
#include "Flourish/Api/RayTracing/AccelerationStructure.h"

namespace Heart
{
    Mesh::Mesh(const HVector<Vertex>& vertices, const HVector<u32>& indices, u32 materialIndex)
        : m_Vertices(vertices), m_Indices(indices), m_MaterialIndex(materialIndex)
    {
        Ref<Flourish::CommandBuffer> uploadBuf = Flourish::CommandBuffer::Create({ false });

        auto uploadEncoder = uploadBuf->EncodeTransferCommands();
        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Type = Flourish::BufferType::Vertex;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Static;
        bufCreateInfo.Layout = s_VertexLayout;
        bufCreateInfo.ElementCount = vertices.Count();
        bufCreateInfo.InitialData = vertices.Data();
        bufCreateInfo.InitialDataSize = sizeof(Vertex) * vertices.Count();
        bufCreateInfo.UploadEncoder = uploadEncoder;
        if (Flourish::Context::FeatureTable().RayTracing)
        {
            bufCreateInfo.CanCreateAccelerationStructure = true;
            bufCreateInfo.ExposeGPUAddress = true;
        }
        m_VertexBuffer = Flourish::Buffer::Create(bufCreateInfo);

        bufCreateInfo.Type = Flourish::BufferType::Index;
        bufCreateInfo.Layout = s_IndexLayout;
        bufCreateInfo.ElementCount = indices.Count();
        bufCreateInfo.InitialData = indices.Data();
        bufCreateInfo.InitialDataSize = sizeof(u32) * indices.Count();
        m_IndexBuffer = Flourish::Buffer::Create(bufCreateInfo);
        uploadEncoder->EndEncoding();

        Ref<Flourish::RenderGraph> graph = Flourish::RenderGraph::Create({ Flourish::RenderGraphUsageType::Once });
        auto builder = graph->ConstructNewNode(uploadBuf.get());
        builder.AddEncoderNode(Flourish::GPUWorkloadType::Transfer)
            .EncoderAddBufferWrite(m_VertexBuffer.get())
            .EncoderAddBufferWrite(m_IndexBuffer.get());
        if (Flourish::Context::FeatureTable().RayTracing)
        {
            builder.AddEncoderNode(Flourish::GPUWorkloadType::Compute)
                .EncoderAddBufferRead(m_VertexBuffer.get())
                .EncoderAddBufferRead(m_IndexBuffer.get());
        }
        builder.AddToGraph();
        graph->Build();

        if (Flourish::Context::FeatureTable().RayTracing)
        {
            Flourish::AccelerationStructureCreateInfo asCreateInfo;
            asCreateInfo.Type = Flourish::AccelerationStructureType::Node;
            asCreateInfo.AllowUpdating = false;
            auto accelStruct = Flourish::AccelerationStructure::Create(asCreateInfo);

            auto buildEncoder = uploadBuf->EncodeComputeCommands();
            Flourish::AccelerationStructureNodeBuildInfo buildInfo;
            buildInfo.VertexBuffer = m_VertexBuffer.get();
            buildInfo.IndexBuffer = m_IndexBuffer.get();
            buildEncoder->RebuildAccelerationStructureNode(accelStruct.get(), buildInfo);
            buildEncoder->EndEncoding();

            // TODO: push?
            Flourish::Context::ExecuteRenderGraph(graph.get());

            m_AccelStructure = accelStruct;
        }
        else
            Flourish::Context::PushRenderGraph(graph.get());

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
