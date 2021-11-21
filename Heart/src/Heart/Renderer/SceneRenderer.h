#pragma once

#include "Heart/Scene/Scene.h"
#include "Heart/Renderer/GraphicsContext.h"
#include "Heart/Renderer/Framebuffer.h"
#include "Heart/Renderer/Buffer.h"
#include "Heart/Renderer/Texture.h"
#include "Heart/Renderer/Material.h"
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

        void RenderScene(GraphicsContext& context, Scene* scene, glm::mat4 view, glm::mat4 projection, glm::vec3 position);

        inline Framebuffer& GetFinalFramebuffer() { return *m_FinalFramebuffer; }

    private:
        struct CachedRender
        {
            CachedRender(UUID material, UUID mesh, u32 submeshIndex, const ObjectData& objData)
                : Material(material), Mesh(mesh), SubmeshIndex(submeshIndex), ObjectData(objData)
            {}

            UUID Material;
            UUID Mesh;
            u32 SubmeshIndex;
            ObjectData ObjectData;
        };

    private:
        void CalculateEnvironmentMaps(GraphicsContext& context);

    private:
        Ref<Texture> m_EnvironmentMap;
        Ref<Texture> m_IrradianceMap;
        std::vector<Ref<Framebuffer>> m_CubemapFramebuffers;
        std::vector<Ref<Framebuffer>> m_IrradianceMapFramebuffers;
        Ref<Framebuffer> m_FinalFramebuffer;
        Ref<Buffer> m_FrameDataBuffer;
        Ref<Buffer> m_ObjectDataBuffer;
        Ref<Buffer> m_MaterialDataBuffer;
    };
}