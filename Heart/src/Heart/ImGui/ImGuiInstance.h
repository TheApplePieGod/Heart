#pragma once

#include "Heart/Core/Window.h"

namespace Heart
{
    class ImGuiInstance
    {
    public:
        ImGuiInstance(Window& window);
        ~ImGuiInstance();

        void Recreate(GraphicsContext& context);

        // For now, these functions use the Engine's window as opposed to consuming a window, as support for multi windows is likely not needed
        void BeginFrame();
        void EndFrame();

    public:
        static Scope<ImGuiInstance> Create(Window& window);

    private:
        void Cleanup();

    private:
        bool m_Initialized = false;

    };
}