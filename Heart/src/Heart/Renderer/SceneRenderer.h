#pragma once

#include "Heart/Scene/Scene.h"
#include "Heart/Renderer/GraphicsContext.h"
#include "Heart/Renderer/Framebuffer.h"
#include "Heart/Renderer/Buffer.h"
#include "Heart/Renderer/Texture.h"
#include "Heart/Renderer/Material.h"
#include "Heart/Renderer/EnvironmentMap.h"
#include "Heart/Core/Camera.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

namespace Heart
{
    class SceneRenderer
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
            int EntityId = -1;
            glm::vec3 Padding;
        };

    public:
        SceneRenderer();
        ~SceneRenderer();

        void RenderScene(GraphicsContext& context, Scene* scene, const Camera& camera, glm::vec3 cameraPosition, bool drawGrid, EnvironmentMap* envMap = nullptr);

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

    private:
        void RenderEnvironmentMap();
        void RenderGrid();
        void RenderOpaque();
        void RenderTransparent();
        void Composite();

        void InitializeGridBuffers();

    private:
        Ref<Texture> m_DefaultEnvironmentMap;
        Ref<Framebuffer> m_FinalFramebuffer;
        Ref<Buffer> m_FrameDataBuffer;
        Ref<Buffer> m_ObjectDataBuffer;
        Ref<Buffer> m_MaterialDataBuffer;

        // grid
        Ref<Buffer> m_GridVertices;
        Ref<Buffer> m_GridIndices;

        // in-flight frame data
        Scene* m_Scene;
        EnvironmentMap* m_EnvironmentMap;
        std::vector<CachedRender> m_TransparentMeshes;
        u32 m_ObjectDataOffset = 0;
        u32 m_MaterialDataOffset = 0;  
    };
}