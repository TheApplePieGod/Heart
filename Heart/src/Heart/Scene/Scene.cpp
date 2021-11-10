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

    Entity Scene::DuplicateEntity(Entity source)
    {
        auto newEntity = m_Registry.create();

        if (m_Registry.any_of<NameComponent>(source.GetHandle()))
            m_Registry.emplace<NameComponent>(newEntity, m_Registry.get<NameComponent>(source.GetHandle()).Name + " Copy");

        CopyComponent<TransformComponent>(source.GetHandle(), newEntity);
        CopyComponent<MeshComponent>(source.GetHandle(), newEntity);

        return { this, newEntity };
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