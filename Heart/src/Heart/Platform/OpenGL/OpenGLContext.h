#pragma once

#include "Heart/Renderer/GraphicsContext.h"

namespace Heart
{
    class OpenGLGraphicsPipeline;
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

        // inline static void SetBoundVertexTopology(int topology) { s_BoundVertexTopology = topology; }
        // inline static int GetBoundVertexTopology() { return s_BoundVertexTopology; }

        inline static void SetBoundGraphicsPipeline(OpenGLGraphicsPipeline* pipeline) { s_BoundGraphicsPipeline = pipeline; }
        inline static OpenGLGraphicsPipeline* GetBoundGraphicsPipeline() { return s_BoundGraphicsPipeline; }

    private:
        static OpenGLGraphicsPipeline* s_BoundGraphicsPipeline;

    };
}