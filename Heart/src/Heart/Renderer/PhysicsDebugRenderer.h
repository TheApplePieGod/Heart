#pragma once

#include "LinearMath/btIDebugDraw.h"
#include "glm/mat4x4.hpp"

namespace Flourish
{
    class Framebuffer;
    class Buffer;
    class Texture;
    class CommandBuffer;
    class RenderPass;
}

namespace Heart
{
    class Scene;
    class Camera;
    class PhysicsDebugRenderer : public btIDebugDraw
    {
    public:
        PhysicsDebugRenderer(u32 width, u32 height);
        
        void Draw(Scene* scene, const Camera& camera);
        void Resize(u32 width, u32 height);
        
        void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) override;
        void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color) override;
        void reportErrorWarning(const char *warningString) override;
        void draw3dText(const btVector3 &location, const char *textString) override;
        void setDebugMode(int debugMode) override { m_DebugMode = debugMode; }
        int getDebugMode() const override { return m_DebugMode; }
        
        inline Flourish::CommandBuffer* GetCommandBuffer() const { return m_MainCommandBuffer.get(); }
        inline Flourish::Texture* GetFinalTexture() const { return m_FinalTexture.get(); }
        
    private:
        struct CameraData
        {
            glm::mat4 Proj;
            glm::mat4 View;
        };
        
    private:
        void CreateTextures();
        void CreateFramebuffers();
        
    private:
        int m_DebugMode = DBG_NoDebug;
        u32 m_RenderWidth, m_RenderHeight;
        
        Ref<Flourish::Framebuffer> m_MainFramebuffer;
        Ref<Flourish::CommandBuffer> m_MainCommandBuffer;
        Ref<Flourish::RenderPass> m_MainRenderPass;
        Ref<Flourish::Texture> m_FinalTexture;
        Ref<Flourish::Buffer> m_VertexBuffer;
        Ref<Flourish::Buffer> m_CameraDataBuffer;
        
        // Frame data
        u32 m_VertexCount = 0;
    };
}
