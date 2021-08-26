#pragma once

#include "Heart/Renderer/GraphicsContext.h"

namespace Heart
{
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

    private:

    };
}