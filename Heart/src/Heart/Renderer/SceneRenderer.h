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

        void RenderScene(GraphicsContext& context, Scene* scene, glm::mat4 viewProjection);

        inline Framebuffer& GetFinalFramebuffer() { return *m_FinalFramebuffer; }

    private:
        Ref<Buffer> m_VertexBuffer;
        Ref<Buffer> m_IndexBuffer;
        Ref<Framebuffer> m_FinalFramebuffer;
        ShaderRegistry m_ShaderRegistry;
        Ref<Buffer> m_FrameDataBuffer;
        Ref<Buffer> m_ObjectDataBuffer;
        TextureRegistry m_TextureRegistry;
    };
}