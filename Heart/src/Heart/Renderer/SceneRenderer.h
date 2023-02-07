#pragma once

#include "Heart/Events/EventEmitter.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "Heart/Container/HVector.hpp"
#include "Heart/Renderer/PhysicsDebugRenderer.h"
#include "Flourish/Api/Context.h"
#include "Heart/Task/Task.h"

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
        bool SSAOEnable = false;
        float SSAORadius = 0.5f;
        float SSAOBias = 0.025f;
        int SSAOKernelSize = 64;
        float BloomThreshold = 1.f;
        float BloomKnee = 0.1f;
        float BloomSampleScale = 1.f;
        bool CullEnable = true;
        bool AsyncAssetLoading = true;
        bool CopyEntityIdsTextureToCPU = false;
        bool RenderPhysicsVolumes = false;
    };

    class RenderScene;
    class Material;
    class Mesh;
    class EnvironmentMap;
    class Camera;
    class WindowResizeEvent;
    class SceneRenderer : public EventListener
    {
    public:
        SceneRenderer();
        ~SceneRenderer();
        
        void ClearRenderData();

        // Returns update task that must be waited on before RenderScene changes or Render is called again
        Task Render(RenderScene* scene, EnvironmentMap* envMap, const Camera& camera, glm::vec3 cameraPosition, const SceneRenderSettings& renderSettings);

        void OnEvent(Event& event) override;

        inline const auto& GetRenderBuffers() { return m_RenderBuffers; }
        inline Flourish::Texture* GetFinalTexture() { return m_FinalTexture.get(); }
        inline Flourish::Texture* GetRenderOutputTexture() { return m_RenderOutputTexture.get(); }
        inline Flourish::Texture* GetEntityIdsTexture() { return m_EntityIdsTexture.get(); }
        inline Flourish::Texture* GetDepthTexture() { return m_DepthTexture.get(); }
        inline Flourish::Texture* GetSSAOTexture() { return m_SSAOTexture.get(); }
        inline Flourish::Texture* GetBloomUpsampleTexture() { return m_BloomUpsampleBufferTexture.get(); }
        inline Flourish::Texture* GetBloomDownsampleTexture() { return m_BloomDownsampleBufferTexture.get(); }
        inline Flourish::Buffer* GetEntityIdsPixelBuffer() { return m_EntityIdsPixelBuffer.get(); }
        inline u32 GetBloomMipCount() const { return m_BloomMipCount; }
        
        inline u32 GetRenderedInstanceCount() const { return m_BatchRenderData[m_RenderFrameIndex].RenderedInstanceCount; }
        inline u32 GetRenderedObjectCount() const { return m_BatchRenderData[m_RenderFrameIndex].RenderedObjectCount; }
        inline u32 GetBatchCount() const { return m_BatchRenderData[m_RenderFrameIndex].IndirectBatches.size(); }
        
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

        struct FrameData
        {
            glm::mat4 Proj;
            glm::mat4 View;
            glm::mat4 InvViewProj;
            glm::vec4 CameraPos;
            glm::vec2 ScreenSize;
            u32 ReverseDepth;
            u32 CullEnable;
            u32 BloomEnable;
            u32 SSAOEnable;
            u32 PhysicsDebugEnable;
            float Padding;
        };

        struct ObjectData
        {
            glm::mat4 Model;
            glm::vec4 Data;
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

        struct SSAOData
        {
            glm::vec4 Samples[64];
            u32 KernelSize;
            float Radius;
            float Bias;
            float Padding;
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

    private:
        void Initialize();
        void Resize();
        
        void CreateBuffers();
        void CreateTextures();
        void CreateRenderPasses();
        void CreateFramebuffers();
        void CreateComputeObjects();
        void InitializeGridBuffers();
        void InitializeSSAOData();

        void UpdateLightingBuffer();
        void CalculateBatches();
        void BindMaterial(Material* material);
        void BindPBRDefaults();

        bool FrustumCull(glm::vec4 boundingSphere, const glm::mat4& transform);

        void RenderEnvironmentMap();
        void RenderGrid();
        void RenderBatches();
        void RenderText();
        void Composite();
        void CopyEntityIdsTexture();
        void SSAO();
        void Bloom();
        void FinalComposite();

        bool OnWindowResize(WindowResizeEvent& event);

    private:
        const u32 m_MaxBloomMipCount = 7;

        Ref<Flourish::Buffer> m_EntityIdsPixelBuffer;
        Ref<Flourish::Buffer> m_FrameDataBuffer;
        Ref<Flourish::Buffer> m_LightingDataBuffer;
        Ref<Flourish::Texture> m_DepthTexture;
        Ref<Flourish::Texture> m_DefaultEnvironmentMap;
        Ref<Flourish::Texture> m_RenderOutputTexture;
        Ref<Flourish::Texture> m_EntityIdsTexture;
        Ref<Flourish::Framebuffer> m_MainFramebuffer;
        Ref<Flourish::CommandBuffer> m_MainCommandBuffer;
        Ref<Flourish::RenderPass> m_MainRenderPass;

        Ref<Flourish::ComputePipeline> m_BloomDownsampleComputePipeline;
        Ref<Flourish::ComputePipeline> m_BloomUpsampleComputePipeline;
        Ref<Flourish::ComputeTarget> m_BloomComputeTarget;
        Ref<Flourish::CommandBuffer> m_BloomCommandBuffer;
        Ref<Flourish::Texture> m_BloomDownsampleBufferTexture;
        Ref<Flourish::Texture> m_BloomUpsampleBufferTexture;
        Ref<Flourish::Buffer> m_BloomDataBuffer;

        Ref<Flourish::ComputePipeline> m_SSAOComputePipeline;
        Ref<Flourish::ComputeTarget> m_SSAOComputeTarget;
        Ref<Flourish::CommandBuffer> m_SSAOCommandBuffer;
        Ref<Flourish::Texture> m_SSAOTexture;
        Ref<Flourish::Texture> m_SSAONoiseTexture;
        Ref<Flourish::Buffer> m_SSAODataBuffer;
        SSAOData m_SSAOData;
        
        Ref<Flourish::ComputePipeline> m_FinalCompositeComputePipeline;
        Ref<Flourish::ComputeTarget> m_FinalComputeTarget;
        Ref<Flourish::CommandBuffer> m_FinalCommandBuffer;
        Ref<Flourish::Texture> m_FinalTexture;

        Ref<Flourish::Buffer> m_GridVertices;
        Ref<Flourish::Buffer> m_GridIndices;
        
        Ref<PhysicsDebugRenderer> m_PhysicsDebugRenderer;
        
        // In-flight frame data
        Flourish::RenderCommandEncoder* m_RenderEncoder;
        RenderScene* m_Scene = nullptr;
        EnvironmentMap* m_EnvironmentMap;
        const Camera* m_Camera;
        u32 m_UpdateFrameIndex = 0;
        u32 m_RenderFrameIndex = 0;
        std::array<BatchRenderData, Flourish::Context::MaxFrameBufferCount> m_BatchRenderData;
        SceneRenderSettings m_SceneRenderSettings;
        std::vector<std::vector<Flourish::CommandBuffer*>> m_RenderBuffers;

        u32 m_BloomMipCount = 0;
        bool m_ShouldResize = false;
        u32 m_RenderWidth = 0;
        u32 m_RenderHeight = 0;
    };
}
