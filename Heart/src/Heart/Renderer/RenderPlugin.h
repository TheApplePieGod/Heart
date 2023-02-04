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
        RenderPlugin(HStringView8 name)
            : m_Name(name)
        {}
        RenderPlugin(HStringView8 name, const HVector<Ref<RenderPlugin>>& dependencies)
            : m_Name(name), m_Dependencies(dependencies)
        {}

        void Render(const SceneRenderData& data, SceneRenderer2* sceneRenderer);
        
        inline const Task& GetTask() const { return m_Task; }
        inline void SetActive(bool active) { m_Active = active; }
        inline bool IsActive() const { return m_Active; }
        inline HStringView8 GetName() const { return HStringView8(m_Name); }
    
    protected:
        virtual void RenderInternal(const SceneRenderData& data, SceneRenderer2* sceneRenderer) = 0;

    protected:
        Task m_Task;
        bool m_Active = true;
        HString8 m_Name;
        u64 m_LastRenderFrame = 0;
        HVector<Ref<RenderPlugin>> m_Dependencies;
        HVector<Task> m_DependencyTasks;
    };
}