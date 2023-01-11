#pragma once

#include "HeartRuntime/Viewport.h"
#include "HeartRuntime/DevPanel.h"
#include "Heart/Core/Layer.h"
#include "Heart/Scene/Scene.h"
#include "Heart/Events/KeyboardEvents.h"

namespace HeartRuntime
{
    class RuntimeLayer : public Heart::Layer
    {
    public:
        RuntimeLayer(const std::filesystem::path& projectPath);
        ~RuntimeLayer() override;

        void OnAttach() override;
        void OnUpdate(Heart::Timestep ts) override;
        void OnDetach() override;

        void OnEvent(Heart::Event& event) override;

    private:
        void LoadProject();
        bool KeyPressedEvent(Heart::KeyPressedEvent& event);

    private:
        std::filesystem::path m_ProjectPath;
        Heart::Ref<Heart::Scene> m_RuntimeScene;
        Heart::SceneRenderSettings m_RenderSettings;
        Viewport m_Viewport;
        DevPanel m_DevPanel;
    };
}