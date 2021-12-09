#pragma once

#include "Heart/Events/EventEmitter.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

namespace Heart
{
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
        struct ObjectData
        {
            glm::mat4 Model;
            glm::vec4 Data;
        };

    public:
        SceneRenderer();
        ~SceneRenderer();

        void RenderScene(GraphicsContext& context, Scene* scene, const Camera& camera, glm::vec3 cameraPosition, bool drawGrid);

        void OnEvent(Event& event) override;

        inline Framebuffer& GetFinalFramebuffer() { return *m_PostBloomFramebuffer; }

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

    private:
        bool m_Initialized = false;

        Ref<Framebuffer> m_FinalFramebuffer;
        Ref<Framebuffer> m_HorizontalBloomFramebuffer;
        Ref<Framebuffer> m_VerticalBloomFramebuffer;
        Ref<Framebuffer> m_PostBloomFramebuffer;

        Ref<Texture> m_DefaultEnvironmentMap;
        Ref<Texture> m_PreBloomTexture;
        Ref<Texture> m_BloomTexture1;
        Ref<Texture> m_BloomTexture2;

        Ref<Buffer> m_FrameDataBuffer;
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
    };
}