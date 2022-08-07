#pragma once

#include "Heart/Events/EventEmitter.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "Heart/Container/HVector.hpp"

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
    class GraphicsContext;
    class Framebuffer;
    class ComputePipeline;
    class Buffer;
    class Texture;
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

        void RenderScene(GraphicsContext& context, Scene* scene, const Camera& camera, glm::vec3 cameraPosition, const SceneRenderSettings& renderSettings);

        void OnEvent(Event& event) override;

        inline Texture& GetFinalTexture() { return *m_FinalTexture; }
        inline Texture& GetPreBloomTexture() { return *m_PreBloomTexture; }
        inline Texture& GetBrightColorsTexture() { return *m_BrightColorsTexture; }
        inline Texture& GetBloomBuffer1Texture() { return *m_BloomBufferTexture; }
        inline Texture& GetBloomBuffer2Texture() { return *m_BloomUpsampleBufferTexture; }
        inline Texture& GetEntityIdsTexture() { return *m_EntityIdsTexture; }
        inline Framebuffer& GetMainFramebuffer() { return *m_MainFramebuffer; }
        inline ComputePipeline& GetCullPipeline() { return *m_ComputeCullPipeline; }
        inline HVector<std::array<Ref<Framebuffer>, 2>>& GetBloomFramebuffers() { return m_BloomFramebuffers; }

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
        void Bloom(GraphicsContext& context);

        void InitializeGridBuffers();

        bool OnAppGraphicsInit(AppGraphicsInitEvent& event);
        bool OnAppGraphicsShutdown(AppGraphicsShutdownEvent& event);
        bool OnWindowResize(WindowResizeEvent& event);

    private:
        bool m_Initialized = false;

        Ref<Framebuffer> m_MainFramebuffer;
        HVector<std::array<Ref<Framebuffer>, 2>> m_BloomFramebuffers; // one for each mip level and one for horizontal / vertical passes

        Ref<ComputePipeline> m_ComputeCullPipeline;
        Ref<Buffer> m_CullDataBuffer;
        Ref<Buffer> m_InstanceDataBuffer;
        Ref<Buffer> m_FinalInstanceBuffer;
        Ref<Buffer> m_IndirectBuffer;

        Ref<Texture> m_DefaultEnvironmentMap;
        Ref<Texture> m_PreBloomTexture;
        Ref<Texture> m_BrightColorsTexture;
        Ref<Texture> m_BloomBufferTexture;
        Ref<Texture> m_BloomUpsampleBufferTexture;

        Ref<Texture> m_FinalTexture;
        Ref<Texture> m_EntityIdsTexture;

        Ref<Buffer> m_FrameDataBuffer;
        Ref<Buffer> m_BloomDataBuffer;
        Ref<Buffer> m_ObjectDataBuffer;
        Ref<Buffer> m_MaterialDataBuffer;
        Ref<Buffer> m_LightingDataBuffer;

        // grid
        Ref<Buffer> m_GridVertices;
        Ref<Buffer> m_GridIndices;

        // in-flight frame data
        Scene* m_Scene;
        EnvironmentMap* m_EnvironmentMap;
        const Camera* m_Camera;
        std::unordered_map<u64, IndirectBatch> m_IndirectBatches;
        HVector<IndirectBatch*> m_DeferredIndirectBatches;
        HVector<HVector<u32>> m_EntityListPool;
        SceneRenderSettings m_SceneRenderSettings;
        u32 m_RenderedInstanceCount;

        const u32 m_BloomMipCount = 5;
        bool m_ShouldResize = false;
        u32 m_RenderWidth = 0;
        u32 m_RenderHeight = 0;
    };
}