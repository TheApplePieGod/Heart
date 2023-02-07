#pragma once

#include "glm/vec3.hpp"
#include "Heart/Events/EventEmitter.h"
#include "Heart/Task/Task.h"
#include "Heart/Container/HVector.hpp"
#include "Heart/Container/HString8.h"
#include "Heart/Scene/RenderScene.h"
#include "Heart/Core/Camera.h"
#include "Heart/Renderer/EnvironmentMap.h"

namespace Heart
{
    struct SceneRenderSettings2
    {
        bool DrawGrid = true;
        bool BloomEnable = true;
        bool SSAOEnable = false;
        float SSAORadius = 0.5f;
        float SSAOBias = 0.025f;
        int SSAOKernelSize = 64;
        float BloomThreshold = 1.f;
        float BloomKnee = 0.1f;
        float BloomSampleScale = 1.f;
        bool CullEnable = true;
        bool AsyncAssetLoading = true;
        bool CopyEntityIdsTextureToCPU = false;
        bool RenderPhysicsVolumes = false;
    };

    struct SceneRenderData
    {
        const RenderScene* Scene;
        const EnvironmentMap* EnvMap;
        const Camera* Camera;
        glm::vec3 CameraPos;
        SceneRenderSettings2 Settings;
    };
    
    class RenderPlugin;
    class WindowResizeEvent;
    class SceneRenderer2 : public EventListener
    {
    public:
        SceneRenderer2();
        ~SceneRenderer2();

        void OnEvent(Event& event) override;

        TaskGroup Render(const SceneRenderData& data);

        template<typename Plugin, typename ... Args>
        Ref<RenderPlugin> RegisterPlugin(Args&& ... args)
        {
            auto plugin = CreateRef<Plugin>(std::forward<Args>(args)...);
            m_Plugins[plugin->GetName()] = plugin;
            return plugin;
        }

        inline const RenderPlugin* GetPlugin(const HString8& name) const
        {
            return m_Plugins.at(name).get();
        }

        template<typename Plugin>
        inline const Plugin* GetPlugin(const HString8& name) const
        {
            return static_cast<Plugin*>(m_Plugins.at(name).get());
        }

        inline u32 GetRenderWidth() const { return m_RenderWidth; }
        inline u32 GetRenderHeight() const { return m_RenderHeight; }
    
    private:
        bool OnWindowResize(WindowResizeEvent& event);

    private:
        HVector<HString8> m_PluginLeaves;
        std::unordered_map<HString8, Ref<RenderPlugin>> m_Plugins;
        u32 m_RenderWidth, m_RenderHeight;
        bool m_ShouldResize = false;
    };
}