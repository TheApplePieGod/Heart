#pragma once

#include "Heart/Scene/Scene.h"
#include "Heart/Renderer/GraphicsContext.h"
#include "Heart/Renderer/Framebuffer.h"
#include "Heart/Renderer/Buffer.h"
#include "Heart/Renderer/Texture.h"
#include "Heart/Renderer/Material.h"
#include "Heart/Renderer/Mesh.h"
#include "Heart/Renderer/EnvironmentMap.h"
#include "Heart/Core/Camera.h"
#include "Heart/Events/EventEmitter.h"
#include "Heart/Events/AppEvents.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

namespace Heart
{
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
            bool padding;
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

        inline Framebuffer& GetFinalFramebuffer() { return *m_FinalFramebuffer; }

    private:
        struct CachedRender
        {
            CachedRender(Material* material, UUID mesh, u32 submeshIndex, const ObjectData& objData)
                : Material(material), Mesh(mesh), SubmeshIndex(submeshIndex), ObjectData(objData)
            {}

            Material* Material;
            UUID Mesh;
            u32 SubmeshIndex;
            ObjectData ObjectData;
        };
        struct IndirectBatch
        {
            Material* Material;
            Mesh* Mesh;
            u32 First = 0;
            u32 Count = 0;
            std::vector<u32> RenderIds;
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

        void CalculateBatches();
        void RenderOpaqueIndirect();

        void RenderEnvironmentMap();
        void RenderGrid();
        void RenderOpaque();
        void RenderTransparent();
        void Composite();

        void InitializeGridBuffers();

        bool OnAppGraphicsInit(AppGraphicsInitEvent& event);
        bool OnAppGraphicsShutdown(AppGraphicsShutdownEvent& event);

    private:
        bool m_Initialized = false;

        Ref<Texture> m_DefaultEnvironmentMap;
        Ref<Framebuffer> m_FinalFramebuffer;
        Ref<Buffer> m_FrameDataBuffer;
        Ref<Buffer> m_ObjectDataBuffer;
        Ref<Buffer> m_MaterialDataBuffer;
        Ref<Buffer> m_IndirectBuffer;

        // grid
        Ref<Buffer> m_GridVertices;
        Ref<Buffer> m_GridIndices;

        // in-flight frame data
        Scene* m_Scene;
        EnvironmentMap* m_EnvironmentMap;
        std::vector<CachedRender> m_TranslucentMeshes;
        std::unordered_map<u64, IndirectBatch> m_IndirectBatches;
        u32 m_ObjectDataOffset = 0;
        u32 m_MaterialDataOffset = 0;  
    };
}