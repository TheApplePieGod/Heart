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
        return CreateEntityWithUUID(name, UUID());
    }

    Entity Scene::CreateEntityWithUUID(const std::string& name, UUID uuid)
    {
        Entity entity(this, m_Registry.create());

        entity.AddComponent<IdComponent>(uuid);
        entity.AddComponent<NameComponent>(name == "" ? "New Entity" : name);
        entity.AddComponent<TransformComponent>();

        m_UUIDMap[uuid] = entity.GetHandle();

        return entity;
    }

    Entity Scene::DuplicateEntity(Entity source)
    {
        auto newEntity = m_Registry.create();
        UUID newUUID = UUID();

        m_Registry.emplace<IdComponent>(newEntity, newUUID);

        if (m_Registry.any_of<NameComponent>(source.GetHandle()))
            m_Registry.emplace<NameComponent>(newEntity, m_Registry.get<NameComponent>(source.GetHandle()).Name + " Copy");

        CopyComponent<TransformComponent>(source.GetHandle(), newEntity);
        CopyComponent<MeshComponent>(source.GetHandle(), newEntity);

        m_UUIDMap[newUUID] = newEntity;

        return { this, newEntity };
    }

    void Scene::DestroyEntity(Entity entity)
    {
        m_Registry.destroy(entity.GetHandle());
    }

    void Scene::AssignRelationship(Entity parent, Entity child)
    {
        HE_ENGINE_ASSERT(parent.IsValid(), "Parent entity must be valid");
        HE_ENGINE_ASSERT(child.IsValid(), "Child entity must be valid");

        UUID childUUID = child.GetUUID();
        UUID parentUUID = parent.GetUUID();

        if (child.HasComponent<ParentComponent>())
        {
            auto& parentComp = child.GetComponent<ParentComponent>();

            if (parentComp.ParentUUID != parentUUID)
            {
                // remove this child from the original parent
                RemoveChild(GetEntityFromUUID(parentComp.ParentUUID).GetHandle(), childUUID);

                // update the parent component
                parentComp.ParentUUID = parentUUID;
            }
            else
                return;
        }
        else
            child.AddComponent<ParentComponent>(parentUUID);

        // add this child to the new parent
        if (parent.HasComponent<ChildComponent>())
            parent.GetComponent<ChildComponent>().Children.push_back(childUUID);
        else
            parent.AddComponent<ChildComponent>(std::initializer_list<UUID>({ childUUID }));
    }

    void Scene::UnparentEntity(Entity child)
    {
        HE_ENGINE_ASSERT(child.IsValid(), "Child entity must be valid");

        UUID childUUID = child.GetUUID();

        if (child.HasComponent<ParentComponent>())
        {
            auto& parentComp = child.GetComponent<ParentComponent>();

            // remove this child from the original parent
            RemoveChild(GetEntityFromUUID(parentComp.ParentUUID).GetHandle(), childUUID);

            child.RemoveComponent<ParentComponent>();
        }
    }

    void Scene::RemoveChild(entt::entity parent, UUID childUUID)
    {
        if (m_Registry.any_of<ChildComponent>(parent))
        {
            auto& childComp = m_Registry.get<ChildComponent>(parent);
            for (UUID& elem : childComp.Children)
            {
                if (elem == childUUID)
                {
                    elem = childComp.Children.back();
                    childComp.Children.pop_back();
                    break;
                }
            }
        }
    }

    void Scene::ClearScene()
    {
        m_Registry.clear();
    }

    Entity Scene::GetEntityFromUUID(UUID uuid)
    {
        return { this, m_UUIDMap[uuid] };
    }
}