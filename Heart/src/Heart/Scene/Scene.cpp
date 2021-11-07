#include "htpch.h"
#include "Scene.h"

#include "Heart/Scene/Entity.h"
#include "Heart/Scene/Components.h"

namespace Heart
{
    Scene::Scene()
    {
        
    }

    Scene::~Scene()
    {
        
    }

    Entity Scene::CreateEntity(const std::string& name)
    {
        Entity entity(this, m_Registry.create());

        entity.AddComponent<NameComponent>(name == "" ? "New Entity" : name);
        entity.AddComponent<TransformComponent>();

        return entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        m_Registry.destroy(entity.GetHandle());
    }

    void Scene::ClearScene()
    {
        m_Registry.clear();
    }
}