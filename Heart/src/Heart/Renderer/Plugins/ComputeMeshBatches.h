#pragma once

#include "glm/mat4x4.hpp"
#include "Flourish/Api/Context.h"
#include "Heart/Renderer/RenderPlugin.h"

namespace Flourish
{
    class Buffer;
}

namespace Heart
{
    class Mesh;
    class Material;
}

namespace Heart::RenderPlugins
{
    struct ComputeMeshBatchesCreateInfo
    {
        HString8 CollectMaterialsPluginName;
    };

    class ComputeMeshBatches : public RenderPlugin
    {
    public:
        struct IndexedIndirectCommand
        {
            u32 IndexCount;
            u32 InstanceCount;
            u32 FirstIndex;
            int VertexOffset;
            u32 FirstInstance;

            u32 padding1;
            glm::vec2 padding2;
        };

        struct MeshBatch
        {
            const Mesh* Mesh = nullptr;
            u32 First = 0;
            u32 Count = 0;
            u32 EntityListIndex = 0;
            u32 MeshIndex = 0;
            u32 SubmeshIndex = 0;
        };

        struct alignas(16) ObjectData
        {
            glm::mat4 Model;
            u32 MaterialId;
            u32 EntityId;
        };

        struct EntityListEntry
        {
            u32 EntityId;
            u32 MaterialIndex;
            bool IncludeInPrepass;
        };

        struct BatchData
        {
            std::unordered_map<u64, MeshBatch> Batches;
            HVector<HVector<EntityListEntry>> EntityListPool;

            Ref<Flourish::Buffer> IndirectBuffer;
            Ref<Flourish::Buffer> ObjectDataBuffer;

            u32 TotalInstanceCount;
        };

    public:
        ComputeMeshBatches(SceneRenderer* renderer, HStringView8 name, const ComputeMeshBatchesCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        {}

        inline u32 GetMaxObjects() const { return m_MaxObjects; }
        inline const auto& GetBatchData() const { return m_BatchData; }

    protected:
        void InitializeInternal() override;
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override {};
        
    private:
        bool FrustumCull(const SceneRenderData& data, glm::vec4 boundingSphere, const glm::mat4& transform); // True if visible

    private:
        ComputeMeshBatchesCreateInfo m_Info;

        BatchData m_BatchData;
        u32 m_MaxObjects = 10000; // TODO: parameterize
    };
}
