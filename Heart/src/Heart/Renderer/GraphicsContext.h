#pragma once

namespace Heart
{
    class GraphicsContext
    {
    public:
        virtual ~GraphicsContext() = default;

        virtual void InitializeImGui() = 0;
        virtual void ShutdownImGui() = 0;
        virtual void ImGuiBeginFrame() = 0;
        virtual void ImGuiEndFrame() = 0;

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

    public:
        static Ref<GraphicsContext> Create(void* window);

    protected:
        void* m_WindowHandle;
    };
}