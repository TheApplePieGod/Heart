#pragma once

#include "Heart/Renderer/GraphicsContext.h"

namespace Heart
{
    class OpenGLFramebuffer;
    class OpenGLContext : public GraphicsContext
    {
    public:
        OpenGLContext(void* window);
        ~OpenGLContext() override;

        void InitializeImGui() override;
        void ShutdownImGui() override;
        void ImGuiBeginFrame() override;
        void ImGuiEndFrame() override;

        void BeginFrame() override;
        void EndFrame() override;

        inline static void SetBoundFramebuffer(OpenGLFramebuffer* buffer) { s_BoundFramebuffer = buffer; }
        inline static OpenGLFramebuffer* GetBoundFramebuffer() { HE_ENGINE_ASSERT(s_BoundFramebuffer != nullptr, "No framebuffer is bound (forget to call Framebuffer.Bind()?)"); return s_BoundFramebuffer; }
        inline static int MaxMsaaSamples() { return s_MsaaMaxSamples; }

    private:
        static OpenGLFramebuffer* s_BoundFramebuffer;
        static int s_MsaaMaxSamples;

    };
}