#pragma once

#include "Heart/Task/Task.h"
#include "Heart/Container/HString8.h"
#include "Heart/Container/HVector.hpp"
#include "Heart/Core/UUID.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/RenderGraph.h"

namespace Heart
{
    enum class GraphDependencyType
    {
        CPU = 0,
        GPU
    };

    struct SceneRenderData;
    class SceneRenderer;
    class RenderPlugin
    {
    public:
        enum class StatType
        {
            None = 0,
            Int,
            Float,
            Bool,
            TimeMS
        };

        struct GraphData
        {
            std::unordered_set<HString8> Dependencies;
            HVector<HString8> Dependents;
            u32 MaxDepth = 0;
        };

        struct Stat
        {
            StatType Type;
            union
            {
                int Int; // Int
                float Float; // Float, TimeMS
                bool Bool; // Bool
            } Data;
        };

    public:
        RenderPlugin(SceneRenderer* renderer, HStringView8 name)
            : m_Renderer(renderer), m_Name(name)
        {}

        void Initialize();
        void Render(const SceneRenderData& data);
        void Resize();
        
        void AddDependency(const HString8& name, GraphDependencyType depType);
        void AddInitDependency(const HString8& name);

        GraphData& GetGraphData(GraphDependencyType depType);
        
        inline const Task& GetTask() const { return m_Task; }
        inline void SetActive(bool active) { m_Active = active; }
        inline bool IsActive() const { return m_Active; }
        inline const auto& GetName() const { return m_Name; }
        inline const auto& GetStats() const { return m_Stats; }
        inline UUID GetUUID() const { return m_UUID; }
        inline Flourish::CommandBuffer* GetCommandBuffer() const { return m_CommandBuffer.get(); }
        inline const Ref<Flourish::Texture>& GetOutputTexture() const { return m_OutputTexture; }
        inline const auto& GetGraphNodeBuilder() const { return m_GPUGraphNodeBuilder; }
        inline const auto& GetInitDependencies() const { return m_InitDependencies; }
    
    protected:
        virtual void InitializeInternal() = 0;
        virtual void RenderInternal(const SceneRenderData& data) = 0;
        virtual void ResizeInternal() = 0;

    protected:
        Task m_Task;
        bool m_Active = true;
        bool m_Initialized = false;
        HString8 m_Name;
        UUID m_UUID = UUID();
        u64 m_LastRenderFrame = 0;
        u64 m_LastResizeFrame = 0;
        HVector<Task> m_DependencyTasks;
        std::map<HString8, Stat> m_Stats;
        SceneRenderer* m_Renderer;
        Ref<Flourish::CommandBuffer> m_CommandBuffer;
        Ref<Flourish::Texture> m_OutputTexture;
        std::unordered_set<HString8> m_InitDependencies;
        GraphData m_CPUGraphData;
        GraphData m_GPUGraphData;
        Flourish::RenderGraphNodeBuilder m_GPUGraphNodeBuilder;

        friend class SceneRenderer;
    };
}
