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
        float BloomBlurStrength = 0.2f;
        float BloomBlurScale = 1.f;
        float BloomThreshold = 1.f;
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
        inline Flourish::Texture& GetFinalTexture() { return *m_FinalTexture; }
        inline Flourish::Texture& GetPreBloomTexture() { return *m_PreBloomTexture; }
        inline Flourish::Texture& GetBrightColorsTexture() { return *m_BrightColorsTexture; }
        inline Flourish::Texture& GetBloomBuffer1Texture() { return *m_BloomBufferTexture; }
        inline Flourish::Texture& GetBloomBuffer2Texture() { return *m_BloomUpsampleBufferTexture; }
        inline Flourish::Texture& GetEntityIdsTexture() { return *m_EntityIdsTexture; }
        inline Flourish::Framebuffer& GetMainFramebuffer() { return *m_MainFramebuffer; }
        inline Flourish::ComputePipeline& GetCullPipeline() { return *m_ComputeCullPipeline; }
        //inline HVector<std::array<Ref<Flourish::Framebuffer>, 2>>& GetBloomFramebuffers() { return m_BloomFramebuffers; }

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
            bool ReverseDepth;
            float BloomThreshold;
            bool CullEnable;
            bool padding1;
            glm::vec2 padding2;
        };
        struct BloomData
        {
            u32 MipLevel;
            bool ReverseDepth;
            float BlurScale;
            float BlurStrength;
        };
        struct ObjectData
        {
            glm::mat4 Model;
            glm::vec4 Data;
            glm::vec4 BoundingSphere;
        };
        struct CullData
        {
            std::array<glm::vec4, 6> FrustumPlanes;
            glm::vec4 Data;
        };
        struct BloomCommandData
        {
            // One per horizontal & vertical passes
            std::array<Ref<Flourish::RenderPass>, 2> RenderPass;
            std::array<Ref<Flourish::Framebuffer>, 2> Framebuffer;
            std::array<Ref<Flourish::CommandBuffer>, 2> CommandBuffer;
        };

    private:
        void Initialize();
        void Shutdown();
        void Resize();
        void CreateTextures();
        void CleanupTextures();
        void CreateFramebuffers();
        void CleanupFramebuffers();

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

        void InitializeGridBuffers();

        bool OnAppGraphicsInit(AppGraphicsInitEvent& event);
        bool OnAppGraphicsShutdown(AppGraphicsShutdownEvent& event);
        bool OnWindowResize(WindowResizeEvent& event);

    private:
        bool m_Initialized = false;

        Ref<Flourish::Framebuffer> m_MainFramebuffer;
        Ref<Flourish::CommandBuffer> m_MainCommandBuffer;
        Ref<Flourish::RenderPass> m_MainRenderPass;
        HVector<BloomCommandData> m_BloomFramebuffers; // One for each mip level

        Ref<Flourish::ComputePipeline> m_ComputeCullPipeline;
        Ref<Flourish::Buffer> m_CullDataBuffer;
        Ref<Flourish::Buffer> m_InstanceDataBuffer;
        Ref<Flourish::Buffer> m_FinalInstanceBuffer;
        Ref<Flourish::Buffer> m_IndirectBuffer;

        Ref<Flourish::Texture> m_DefaultEnvironmentMap;
        Ref<Flourish::Texture> m_PreBloomTexture;
        Ref<Flourish::Texture> m_BrightColorsTexture;
        Ref<Flourish::Texture> m_BloomBufferTexture;
        Ref<Flourish::Texture> m_BloomUpsampleBufferTexture;

        Ref<Flourish::Texture> m_FinalTexture;
        Ref<Flourish::Texture> m_EntityIdsTexture;

        Ref<Flourish::Buffer> m_FrameDataBuffer;
        Ref<Flourish::Buffer> m_BloomDataBuffer;
        Ref<Flourish::Buffer> m_ObjectDataBuffer;
        Ref<Flourish::Buffer> m_MaterialDataBuffer;
        Ref<Flourish::Buffer> m_LightingDataBuffer;

        // grid
        Ref<Flourish::Buffer> m_GridVertices;
        Ref<Flourish::Buffer> m_GridIndices;

        // in-flight frame data
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

        const u32 m_BloomMipCount = 5;
        bool m_ShouldResize = false;
        u32 m_RenderWidth = 0;
        u32 m_RenderHeight = 0;
    };
}