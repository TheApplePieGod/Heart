#pragma once

#include "Heart/Events/EventEmitter.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

namespace Heart
{
    struct SceneRenderSettings
    {
        bool DrawGrid = true;
        
        bool BloomEnable = true;
        float BloomBlurStrength = 1.f;
        float BloomBlurScale = 1.f;
    };

    class Scene;
    class GraphicsContext;
    class Framebuffer;
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
        struct FrameData
        {
            glm::mat4 Proj;
            glm::mat4 View;
            glm::vec4 CameraPos;
            glm::vec2 ScreenSize;
            bool ReverseDepth;
            float padding;
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
        };

    public:
        SceneRenderer();
        ~SceneRenderer();

        void RenderScene(GraphicsContext& context, Scene* scene, const Camera& camera, glm::vec3 cameraPosition, const SceneRenderSettings& renderSettings);

        void OnEvent(Event& event) override;

        inline Framebuffer& GetFinalFramebuffer() { return *m_FinalFramebuffer; }
        inline Texture& GetFinalTexture() { return *m_FinalTexture; }

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

        Ref<Framebuffer> m_FinalFramebuffer;
        std::vector<std::array<Ref<Framebuffer>, 2>> m_BloomFramebuffers; // one for each mip level and one for horizontal / vertical passes

        Ref<Texture> m_DefaultEnvironmentMap;
        Ref<Texture> m_PreBloomTexture;
        Ref<Texture> m_BrightColorsTexture;
        Ref<Texture> m_BloomBufferTexture;
        Ref<Texture> m_BloomUpsampleBufferTexture;
        Ref<Texture> m_FinalTexture;

        Ref<Buffer> m_FrameDataBuffer;
        Ref<Buffer> m_BloomDataBuffer;
        Ref<Buffer> m_ObjectDataBuffer;
        Ref<Buffer> m_MaterialDataBuffer;
        Ref<Buffer> m_LightingDataBuffer;
        Ref<Buffer> m_IndirectBuffer;

        // grid
        Ref<Buffer> m_GridVertices;
        Ref<Buffer> m_GridIndices;

        // in-flight frame data
        Scene* m_Scene;
        EnvironmentMap* m_EnvironmentMap;
        std::unordered_map<u64, IndirectBatch> m_IndirectBatches;
        std::vector<IndirectBatch*> m_DeferredIndirectBatches;
        std::vector<std::vector<u32>> m_EntityListPool;
        SceneRenderSettings m_SceneRenderSettings;

        const u32 m_BloomMipCount = 5;
        bool m_ShouldResize = false;
        u32 m_RenderWidth = 0;
        u32 m_RenderHeight = 0;
    };
}