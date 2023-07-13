#pragma once

#include "Heart/Container/HVector.hpp"
#include "Heart/Scene/Components.h"
#include "Heart/Scene/Scene.h"
#include "glm/mat4x4.hpp"
#include "entt/entt.hpp"

namespace Heart
{
    class Scene;
    class RenderScene
    {
    public:
        RenderScene() = default;

        void Cleanup();
        
        void CopyFromScene(Scene* scene);
        
        inline const auto& GetRegistry() const { return m_Registry; }
        inline const auto& GetCachedTransforms() const { return m_CachedTransforms; }

    private:
        void ComputeTextRenderData();

    private:
        entt::registry m_Registry;
        std::unordered_map<entt::entity, Scene::CachedTransformData> m_CachedTransforms;
    };
}
