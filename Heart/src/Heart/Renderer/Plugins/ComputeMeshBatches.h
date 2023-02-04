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
    struct IndirectBatch
    {
        Material* Material;
        Mesh* Mesh;
        u32 First = 0;
        u32 Count = 0;
        u32 EntityListIndex = 0;
    };

    struct BatchRenderData
    {
        std::unordered_map<u64, IndirectBatch> IndirectBatches;
        HVector<IndirectBatch*> DeferredIndirectBatches;
        HVector<HVector<u32>> EntityListPool;

        Ref<Flourish::Buffer> IndirectBuffer;
        Ref<Flourish::Buffer> ObjectDataBuffer;
        Ref<Flourish::Buffer> MaterialDataBuffer;

        u32 RenderedInstanceCount;
        u32 RenderedObjectCount;
    };

    class ComputeMeshBatches : public RenderPlugin
    {
    public:
        ComputeMeshBatches(HStringView8 name)
            : RenderPlugin(name)
        {}
        ComputeMeshBatches(HStringView8 name, const HVector<Ref<RenderPlugin>>& dependencies)
            : RenderPlugin(name, dependencies)
        {}

    protected:
        void RenderInternal(const SceneRenderData& data, SceneRenderer2* sceneRenderer) override;
        
    private:
        bool FrustumCull(const SceneRenderData& data, glm::vec4 boundingSphere, const glm::mat4& transform); // True if visible

    private:
        std::array<BatchRenderData, Flourish::Context::MaxFrameBufferCount> m_BatchRenderData;
        u32 m_UpdateFrameIndex = 0;
        u32 m_RenderFrameIndex = 0;
    };
}