#pragma once

#include "Heart/Events/EventEmitter.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "Heart/Container/HVector.hpp"

namespace Flourish
{
    class Framebuffer;
    class ComputePipeline;
    class ComputeTarget;
    class Buffer;
    class Texture;
    class CommandBuffer;
    class RenderPass;
    class RenderCommandEncoder;
}

namespace Heart
{
    struct SceneRenderSettings
    {
        bool DrawGrid = true;
        bool BloomEnable = true;
        float BloomThreshold = 1.f;
        float BloomKnee = 0.1f;
        float BloomSampleScale = 1.f;
        bool CullEnable = true;
        bool AsyncAssetLoading = true;
        bool CopyEntityIdsTextureToCPU = false;
    };

    class Scene;
    class Material;
    class Mesh;
    class EnvironmentMap;
    class Camera;
    class AppGraphicsInitEvent;
    class AppGraphicsShutdownEvent;
    class WindowResizeEvent;
    class SceneRenderer : public EventListener
    {
    public:
        SceneRenderer();
        ~SceneRenderer();

        void RenderScene(Scene* scene, const Camera& camera, glm::vec3 cameraPosition, const SceneRenderSettings& renderSettings);

        void OnEvent(Event& event) override;

        inline const auto& GetRenderBuffers() { return m_RenderBuffers; }
        inline Flourish::Texture* GetFinalTexture() { return m_FinalTexture.get(); }
        inline Flourish::Texture* GetRenderOutputTexture() { return m_RenderOutputTexture.get(); }
        inline Flourish::Texture* GetEntityIdsTexture() { return m_EntityIdsTexture.get(); }
        inline Flourish::Texture* GetBloomUpsampleTexture() { return m_BloomUpsampleBufferTexture.get(); }
        inline Flourish::Texture* GetBloomDownsampleTexture() { return m_BloomDownsampleBufferTexture.get(); }

        inline u32 GetBloomMipCount() const { return m_BloomMipCount; }

    private:
        struct IndirectBatch
        {
            Material* Material;
            Mesh* Mesh;
            u32 First = 0;
            u32 Count = 0;
            u32 EntityListIndex = 0;
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

        struct InstanceData
        {
            u32 ObjectId;
            u32 BatchId;
            glm::vec2 padding;  
        };

        struct FrameData
        {
            glm::mat4 Proj;
            glm::mat4 View;
            glm::vec4 CameraPos;
            glm::vec2 ScreenSize;
            u32 ReverseDepth;
            u32 CullEnable;
            u32 BloomEnable;
            float padding;
            glm::vec2 padding2;
        };

        struct ObjectData
        {
            glm::mat4 Model;
            glm::vec4 Data;
            glm::vec4 BoundingSphere;
        };

        struct LightData
        {
            glm::vec4 Position;
            glm::vec4 Direction;
            glm::vec4 Color;
            u32 LightType;
            float ConstantAttenuation;
            float LinearAttenuation;
            float QuadraticAttenuation;
        };

        struct CullData
        {
            std::array<glm::vec4, 6> FrustumPlanes;
            glm::vec4 Data;
        };

        struct BloomData
        {
            glm::vec2 SrcResolution;
            glm::vec2 DstResolution;
            float Threshold;
            float Knee;
            float SampleScale;
            u32 Prefilter;
            glm::vec4 Padding1;
            glm::vec4 Padding2;
        };

    private:
        void Initialize();
        void Resize();
        
        void CreateBuffers();
        void CreateTextures();
        void CreateRenderPasses();
        void CreateFramebuffers();
        void CreateComputeObjects();

        void UpdateLightingBuffer();
        void CalculateBatches();
        void BindMaterial(Material* material);
        void BindPBRDefaults();

        void SetupCullCompute();
        void RenderEnvironmentMap();
        void RenderGrid();
        void RenderBatches();
        void Composite();
        void Bloom();
        void FinalComposite();

        void InitializeGridBuffers();

        bool OnAppGraphicsInit(AppGraphicsInitEvent& event);
        bool OnAppGraphicsShutdown(AppGraphicsShutdownEvent& event);
        bool OnWindowResize(WindowResizeEvent& event);

    private:
        Ref<Flourish::Buffer> m_FrameDataBuffer;
        Ref<Flourish::Buffer> m_ObjectDataBuffer;
        Ref<Flourish::Buffer> m_MaterialDataBuffer;
        Ref<Flourish::Buffer> m_LightingDataBuffer;
        Ref<Flourish::Texture> m_DefaultEnvironmentMap;
        Ref<Flourish::Texture> m_RenderOutputTexture;
        Ref<Flourish::Texture> m_EntityIdsTexture;
        Ref<Flourish::Framebuffer> m_MainFramebuffer;
        Ref<Flourish::CommandBuffer> m_MainCommandBuffer;
        Ref<Flourish::RenderPass> m_MainRenderPass;

        Ref<Flourish::ComputePipeline> m_ComputeCullPipeline;
        Ref<Flourish::ComputeTarget> m_CullComputeTarget;
        Ref<Flourish::Buffer> m_CullDataBuffer;
        Ref<Flourish::Buffer> m_InstanceDataBuffer;
        Ref<Flourish::Buffer> m_FinalInstanceBuffer;
        Ref<Flourish::Buffer> m_IndirectBuffer;

        Ref<Flourish::ComputePipeline> m_BloomDownsampleComputePipeline;
        Ref<Flourish::ComputePipeline> m_BloomUpsampleComputePipeline;
        Ref<Flourish::ComputeTarget> m_BloomComputeTarget;
        Ref<Flourish::CommandBuffer> m_BloomCommandBuffer;
        Ref<Flourish::Texture> m_BloomDownsampleBufferTexture;
        Ref<Flourish::Texture> m_BloomUpsampleBufferTexture;
        Ref<Flourish::Buffer> m_BloomDataBuffer;

        Ref<Flourish::ComputePipeline> m_FinalCompositeComputePipeline;
        Ref<Flourish::ComputeTarget> m_FinalComputeTarget;
        Ref<Flourish::CommandBuffer> m_FinalCommandBuffer;
        Ref<Flourish::Texture> m_FinalTexture;

        Ref<Flourish::Buffer> m_GridVertices;
        Ref<Flourish::Buffer> m_GridIndices;

        // In-flight frame data
        Flourish::RenderCommandEncoder* m_RenderEncoder;
        Scene* m_Scene;
        EnvironmentMap* m_EnvironmentMap;
        const Camera* m_Camera;
        std::unordered_map<u64, IndirectBatch> m_IndirectBatches;
        HVector<IndirectBatch*> m_DeferredIndirectBatches;
        HVector<HVector<u32>> m_EntityListPool;
        SceneRenderSettings m_SceneRenderSettings;
        u32 m_RenderedInstanceCount;
        std::vector<std::vector<Flourish::CommandBuffer*>> m_RenderBuffers;

        const u32 m_BloomMipCount = 7;
        bool m_ShouldResize = false;
        u32 m_RenderWidth = 0;
        u32 m_RenderHeight = 0;
    };
}