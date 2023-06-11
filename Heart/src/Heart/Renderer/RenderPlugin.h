#pragma once

#include "Heart/Task/Task.h"
#include "Heart/Container/HString8.h"
#include "Heart/Container/HVector.hpp"
#include "Heart/Core/UUID.h"

namespace Heart
{
    struct SceneRenderData;
    class SceneRenderer2;
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
        RenderPlugin(SceneRenderer2* renderer, HStringView8 name)
            : m_Renderer(renderer), m_Name(name)
        {}

        void Render(const SceneRenderData& data);
        void Resize();
        
        inline void AddDependency(const Ref<RenderPlugin>& plugin) { m_Dependencies.AddInPlace(plugin); }
        
        inline const Task& GetTask() const { return m_Task; }
        inline void SetActive(bool active) { m_Active = active; }
        inline bool IsActive() const { return m_Active; }
        inline HStringView8 GetName() const { return HStringView8(m_Name); }
        inline const auto& GetStats() const { return m_Stats; }
        inline const auto& GetDependencies() const { return m_Dependencies; }
        inline const auto& GetDependents() const { return m_Dependents; }
        inline u32 GetMaxDepth() const { return m_MaxDepth; }
        inline UUID GetUUID() const { return m_UUID; }
    
    protected:
        virtual void RenderInternal(const SceneRenderData& data) = 0;
        virtual void ResizeInternal() = 0;

    protected:
        Task m_Task;
        bool m_Active = true;
        HString8 m_Name;
        UUID m_UUID = UUID();
        u64 m_LastRenderFrame = 0;
        HVector<Ref<RenderPlugin>> m_Dependencies;
        HVector<Task> m_DependencyTasks;
        std::map<HString8, Stat> m_Stats;
        SceneRenderer2* m_Renderer;

    // Graph data
    private:
        // Id here instead of ref to prevent circular dependencies
        HVector<HString8> m_Dependents;
        u32 m_MaxDepth = 0;

        friend class SceneRenderer2;
    };
}
