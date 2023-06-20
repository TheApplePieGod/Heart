#pragma once

#include "glm/mat4x4.hpp"
#include "Flourish/Api/Context.h"
#include "Heart/Core/UUID.h"
#include "Heart/Renderer/RenderPlugin.h"

namespace Flourish
{
    class Buffer;
    class Texture;
}

namespace Heart
{
    class Mesh;
    class Material;
}

namespace Heart::RenderPlugins
{
    struct ComputeTextBatchesCreateInfo
    {

    };

    class ComputeTextBatches : public RenderPlugin
    {
    public:
        enum class BatchType : u8
        {
            Opaque = 0,
            Alpha
        };

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

        struct TextBatch
        {
            Mesh* Mesh = nullptr;
            Material* Material = nullptr;
            Flourish::Texture* FontAtlas = nullptr;
            u32 First = 0;
            u32 Count = 0;
        };

        struct ObjectData
        {
            glm::mat4 Model;
            glm::vec4 Data;
        };

        struct ComputedData
        {
            HVector<TextBatch> Batches;

            Ref<Flourish::Buffer> IndirectBuffer;
            Ref<Flourish::Buffer> ObjectDataBuffer;
            Ref<Flourish::Buffer> MaterialDataBuffer;

            u32 TotalInstanceCount;
        };

    public:
        ComputeTextBatches(SceneRenderer* renderer, HStringView8 name, const ComputeTextBatchesCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        { Initialize(); }

        inline u32 GetMaxObjects() const { return m_MaxObjects; }
        inline const auto& GetComputedData() const { return m_ComputedData[m_RenderFrameIndex]; }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override {};

    private:
        void Initialize();

    private:
        ComputeTextBatchesCreateInfo m_Info;
        std::array<ComputedData, Flourish::Context::MaxFrameBufferCount> m_ComputedData;
        u32 m_UpdateFrameIndex = 0;
        u32 m_RenderFrameIndex = 0;
        u32 m_MaxObjects = 3000; // TODO: parameterize
        u32 m_MaxMaterials = 3000;
    };
}
