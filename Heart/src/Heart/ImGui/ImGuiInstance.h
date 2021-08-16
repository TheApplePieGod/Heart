#pragma once

namespace Heart
{
    class ImGuiInstance
    {
    public:
        ImGuiInstance();
        ~ImGuiInstance();

        void Recreate();

    public:
        static Scope<ImGuiInstance> Create();

    private:
        void Cleanup();

    private:
        bool m_Initialized = false;

    };
}