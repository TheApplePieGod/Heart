#pragma once

#include "Heart/Scene/Scene.h"
#include "Heart/Renderer/GraphicsContext.h"
#include "Heart/Renderer/Framebuffer.h"
#include "Heart/Renderer/Buffer.h"
#include "Heart/Renderer/Texture.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

namespace Heart
{
    class SceneRenderer
    {
    public:
        struct Vertex
        {
            glm::vec3 position;
            glm::vec2 texCoord;
        };
        struct FrameData
        {
            glm::mat4 viewProj;
            glm::mat4 view;
        };
        struct ObjectData
        {
            glm::mat4 model;
            int entityId = -1;
            glm::vec3 padding;
        };

    public:
        SceneRenderer();
        ~SceneRenderer();

        void RenderScene(GraphicsContext& context, Scene* scene, glm::mat4 view, glm::mat4 viewProjection);

        inline Framebuffer& GetFinalFramebuffer() { return *m_FinalFramebuffer; }

    private:
        struct CachedRender
        {
            CachedRender(const std::string& texPath, const std::string& meshPath, u32 submeshIndex, const ObjectData& objData)
                : TexturePath(texPath), MeshPath(meshPath), SubmeshIndex(submeshIndex), ObjectData(objData)
            {}

            std::string TexturePath;
            std::string MeshPath;
            u32 SubmeshIndex;
            ObjectData ObjectData;
        };

    private:
        Ref<Framebuffer> m_FinalFramebuffer;
        Ref<Buffer> m_FrameDataBuffer;
        Ref<Buffer> m_ObjectDataBuffer;
    };
}