#pragma once

#include "glm/mat4x4.hpp"
#include "Flourish/Api/Context.h"
#include "Heart/Core/UUID.h"
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
    struct ComputeMaterialBatchesCreateInfo
    {
        HString8 MeshBatchesPluginName;
    };

    class ComputeMaterialBatches : public RenderPlugin
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

        struct MaterialBatch
        {
            Mesh* Mesh = nullptr;
            Material* Material = nullptr;
            u32 First = 0;
            u32 Count = 0;
        };

        struct ObjectData
        {
            glm::mat4 Model;
            glm::vec4 Data;
        };

        struct BatchData
        {
            HVector<MaterialBatch> Batches;

            Ref<Flourish::Buffer> IndirectBuffer;
            Ref<Flourish::Buffer> ObjectDataBuffer;
            Ref<Flourish::Buffer> MaterialDataBuffer;

            u32 TotalInstanceCount;
        };

    public:
        ComputeMaterialBatches(SceneRenderer2* renderer, HStringView8 name, const ComputeMaterialBatchesCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        { Initialize(); }

        inline u32 GetMaxObjects() const { return m_MaxObjects; }
        inline const auto& GetBatchData() const { return m_BatchData[m_RenderFrameIndex]; }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override {};

    private:
        void Initialize();

    private:
        ComputeMaterialBatchesCreateInfo m_Info;
        std::array<BatchData, Flourish::Context::MaxFrameBufferCount> m_BatchData;
        u32 m_UpdateFrameIndex = 0;
        u32 m_RenderFrameIndex = 0;
        u32 m_MaxObjects = 10000; // TODO: parameterize
        u32 m_MaxMaterials = 10000;
    };
}
