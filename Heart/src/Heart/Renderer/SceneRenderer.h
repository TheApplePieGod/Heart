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
        bool TonemapEnable = true;
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
        SceneRenderer(bool debug);
        virtual ~SceneRenderer();

        TaskGroup Render(const SceneRenderData& data);

        void OnEvent(Event& event) override;

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

        inline void QueueGraphRebuild() { m_ShouldRebuild = true; }

        inline const auto& GetPlugins() const { return m_Plugins; }
        inline u32 GetRenderWidth() const { return m_RenderWidth; }
        inline u32 GetRenderHeight() const { return m_RenderHeight; }
        inline Ref<Flourish::Texture>& GetOutputTexture() { return m_OutputTexture; }
        inline Flourish::Texture* GetDefaultEnvironmentMap() { return m_DefaultEnvironmentMap.get(); }
        inline Flourish::RenderGraph* GetRenderGraph() { return m_RenderGraph.get(); }
        inline bool IsDebug() const { return m_Debug; }

    protected:
        virtual void RegisterPlugins() = 0;
        virtual void CreateResources() = 0;

    protected:
        u32 m_RenderWidth, m_RenderHeight;

        Ref<Flourish::Texture> m_OutputTexture;

        // TODO: move this out further
        Ref<Flourish::Texture> m_DefaultEnvironmentMap;
    
    private:
        bool OnWindowResize(WindowResizeEvent& event);
        void Initialize();
        void InitializeRegisteredPlugins();
        void CreateDefaultResources();
        void Resize();
        void RebuildGraph();
        void RebuildGraphInternal(GraphDependencyType depType);

    private:
        std::unordered_map<HString8, Ref<RenderPlugin>> m_Plugins;
        bool m_ShouldResize = false;
        bool m_ShouldRebuild = false;
        bool m_Debug = false;
        GraphData m_CPUGraphData;
        GraphData m_GPUGraphData;

        Ref<Flourish::RenderGraph> m_RenderGraph;
    };
}
