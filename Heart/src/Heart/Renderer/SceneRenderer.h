#pragma once

#include "glm/vec3.hpp"
#include "Heart/Events/EventEmitter.h"
#include "Heart/Task/Task.h"
#include "Heart/Container/HVector.hpp"
#include "Heart/Container/HString8.h"
#include "Heart/Scene/RenderScene.h"
#include "Heart/Core/Camera.h"
#include "Heart/Renderer/EnvironmentMap.h"
#include "Heart/Renderer/RenderPlugin.h"

namespace Heart
{
    struct SceneRenderSettings
    {
        bool DrawGrid = true;
        bool BloomEnable = true;
        bool SSAOEnable = true;
        float SSAORadius = 0.5f;
        float SSAOBias = 0.025f;
        int SSAOKernelSize = 64;
        float BloomThreshold = 0.25f;
        float BloomKnee = 0.1f;
        float BloomSampleScale = 1.f;
        float BloomStrength = 0.3f;
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
        SceneRenderSettings Settings;
    };
    
    class WindowResizeEvent;
    class SceneRenderer : public EventListener
    {
    public:
        struct GraphData
        {
            HVector<HString8> Leaves;
            HVector<HString8> Roots;
            u32 MaxDepth = 0;
        };

    public:
        SceneRenderer();
        ~SceneRenderer();

        void OnEvent(Event& event) override;

        void RebuildGraph();
        TaskGroup Render(const SceneRenderData& data);

        GraphData& GetGraphData(GraphDependencyType depType);

        template<typename Plugin, typename ... Args>
        Ref<Plugin> RegisterPlugin(Args&& ... args)
        {
            auto plugin = CreateRef<Plugin>(this, std::forward<Args>(args)...);
            m_Plugins[plugin->GetName()] = plugin;
            return plugin;
        }

        inline RenderPlugin* GetPlugin(const HString8& name)
        {
            return m_Plugins[name].get();
        }

        template<typename Plugin>
        inline Plugin* GetPlugin(const HString8& name)
        {
            return static_cast<Plugin*>(m_Plugins[name].get());
        }

        inline const auto& GetPlugins() const { return m_Plugins; }
        inline u32 GetRenderWidth() const { return m_RenderWidth; }
        inline u32 GetRenderHeight() const { return m_RenderHeight; }
        inline Ref<Flourish::Texture>& GetRenderTexture() { return m_RenderTexture; }
        inline Ref<Flourish::Texture>& GetOutputTexture() { return m_OutputTexture; }
        inline Ref<Flourish::Texture>& GetDepthTexture() { return m_DepthTexture; }
        inline Flourish::Texture* GetDefaultEnvironmentMap() { return m_DefaultEnvironmentMap.get(); }
        inline Flourish::RenderGraph* GetRenderGraph() { return m_RenderGraph.get(); }
    
    private:
        bool OnWindowResize(WindowResizeEvent& event);
        void InitializePlugins();
        void CreateTextures();
        void CreateDefaultResources();
        void Resize();
        void RebuildGraphInternal(GraphDependencyType depType);

    private:
        std::unordered_map<HString8, Ref<RenderPlugin>> m_Plugins;
        u32 m_RenderWidth, m_RenderHeight;
        bool m_ShouldResize = false;
        GraphData m_CPUGraphData;
        GraphData m_GPUGraphData;

        Ref<Flourish::Texture> m_RenderTexture;
        Ref<Flourish::Texture> m_OutputTexture;
        Ref<Flourish::Texture> m_DepthTexture;

        // TODO: move this out further
        Ref<Flourish::Texture> m_DefaultEnvironmentMap;

        Ref<Flourish::RenderGraph> m_RenderGraph;
    };
}
