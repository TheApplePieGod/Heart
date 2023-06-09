#pragma once

#include "Heart/Task/Task.h"
#include "Heart/Container/HString8.h"
#include "Heart/Container/HVector.hpp"

namespace Heart
{
    struct SceneRenderData;
    class SceneRenderer2;
    class RenderPlugin
    {
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
    
    protected:
        virtual void RenderInternal(const SceneRenderData& data) = 0;
        virtual void ResizeInternal() = 0;

    protected:
        Task m_Task;
        bool m_Active = true;
        HString8 m_Name;
        u64 m_LastRenderFrame = 0;
        HVector<Ref<RenderPlugin>> m_Dependencies;
        HVector<Task> m_DependencyTasks;
        SceneRenderer2* m_Renderer;
    };
}
