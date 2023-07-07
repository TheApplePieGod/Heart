#pragma once

#include "glm/mat4x4.hpp"
#include "Flourish/Api/Context.h"
#include "Heart/Core/UUID.h"
#include "Heart/Renderer/RenderPlugin.h"
#include "Heart/Renderer/Plugins/ComputeMeshBatches.h"

namespace Flourish
{
    class Buffer;
    class Texture;
}

namespace Heart::RenderPlugins
{
    struct ComputeTextBatchesCreateInfo
    {
        HString8 CollectMaterialsPluginName;
    };

    class ComputeTextBatches : public RenderPlugin
    {
    public:
        struct TextBatch
        {
            const Mesh* Mesh = nullptr;
            const Material* Material = nullptr;
            Flourish::Texture* FontAtlas = nullptr;
            u32 First = 0;
            u32 Count = 0;
        };

        struct BatchData
        {
            HVector<TextBatch> Batches;

            Ref<Flourish::Buffer> IndirectBuffer;
            Ref<Flourish::Buffer> ObjectDataBuffer;

            u32 TotalInstanceCount;
        };

    public:
        ComputeTextBatches(SceneRenderer* renderer, HStringView8 name, const ComputeTextBatchesCreateInfo& createInfo)
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
        ComputeTextBatchesCreateInfo m_Info;
        std::array<BatchData, Flourish::Context::MaxFrameBufferCount> m_BatchData;
        u32 m_UpdateFrameIndex = 0;
        u32 m_RenderFrameIndex = 0;
        u32 m_MaxObjects = 3000; // TODO: parameterize
        u32 m_MaxMaterials = 3000;
    };
}
