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
            Mesh* Mesh;
            u32 First = 0;
            u32 Count = 0;
            u32 EntityListIndex = 0;
        };

        struct ObjectData
        {
            glm::mat4 Model;
        };

    struct BatchData
    {
        std::unordered_map<u64, MeshBatch> Batches;
        HVector<HVector<u32>> EntityListPool;

        Ref<Flourish::Buffer> IndirectBuffer;
        Ref<Flourish::Buffer> ObjectDataBuffer;

        u32 RenderedInstanceCount;
        u32 RenderedObjectCount;
    };

    public:
        ComputeMeshBatches(HStringView8 name)
            : RenderPlugin(name)
        { Initialize(); }

        void Resize(u32 width, u32 height) override {}
        
        inline u32 GetMaxObjects() const { return m_MaxObjects; }
        inline const auto& GetBatchData() const { return m_BatchData[m_RenderFrameIndex]; }

    protected:
        void RenderInternal(const SceneRenderData& data, SceneRenderer2* sceneRenderer) override;
        
    private:
        void Initialize();
        bool FrustumCull(const SceneRenderData& data, glm::vec4 boundingSphere, const glm::mat4& transform); // True if visible

    private:
        std::array<BatchData, Flourish::Context::MaxFrameBufferCount> m_BatchData;
        u32 m_UpdateFrameIndex = 0;
        u32 m_RenderFrameIndex = 0;
        u32 m_MaxObjects = 10000; // TODO: parameterize
    };
}