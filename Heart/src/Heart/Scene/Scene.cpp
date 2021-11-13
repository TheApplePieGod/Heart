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
        Entity entity = { this, m_Registry.create() };
        m_UUIDMap[uuid] = entity.GetHandle();

        entity.AddComponent<IdComponent>(uuid);
        entity.AddComponent<NameComponent>(name == "" ? "New Entity" : name);
        entity.AddComponent<TransformComponent>();

        CacheEntityTransform(entity);

        return entity;
    }

    Entity Scene::DuplicateEntity(Entity source, bool keepParent, bool keepChildren)
    {
        auto newEntityHandle = m_Registry.create();
        UUID newUUID = UUID();
        Entity newEntity = { this, newEntityHandle };
        m_UUIDMap[newUUID] = newEntityHandle;

        m_Registry.emplace<IdComponent>(newEntityHandle, newUUID);

        if (source.HasComponent<NameComponent>())
            m_Registry.emplace<NameComponent>(newEntityHandle, m_Registry.get<NameComponent>(source.GetHandle()).Name + " Copy");

        if (keepParent && source.HasComponent<ParentComponent>())
            AssignRelationship(GetEntityFromUUID(source.GetComponent<ParentComponent>().ParentUUID), newEntity);

        if (keepChildren && source.HasComponent<ChildComponent>())
        {
            auto& childComp = source.GetComponent<ChildComponent>();

            for (auto& child : childComp.Children)
            {
                Entity entity = DuplicateEntity(GetEntityFromUUID(child), false, true);
                AssignRelationship(newEntity, entity);
            }
        }

        CopyComponent<TransformComponent>(source.GetHandle(), newEntityHandle);
        CopyComponent<MeshComponent>(source.GetHandle(), newEntityHandle);

        CacheEntityTransform(newEntity);

        return newEntity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        // remove entity from parent's children
        UnparentEntity(entity);

        DestroyChildren(entity);

        entity.Destroy();
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
                RemoveChild(parentComp.ParentUUID, childUUID);

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
            RemoveChild(parentComp.ParentUUID, childUUID);

            child.RemoveComponent<ParentComponent>();
        }
    }

    void Scene::RemoveChild(UUID parentUUID, UUID childUUID)
    {
        Entity parent = GetEntityFromUUID(parentUUID);

        if (parent.HasComponent<ChildComponent>())
        {
            auto& childComp = parent.GetComponent<ChildComponent>();
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

    void Scene::DestroyChildren(Entity parent)
    {
        // recursively destroy children
        if (parent.HasComponent<ChildComponent>())
        {
            auto& childComp = parent.GetComponent<ChildComponent>();

            for (auto& child : childComp.Children)
            {
                Entity entity = GetEntityFromUUID(child);
                DestroyChildren(entity);
                entity.Destroy();
            }
        }
    }

    glm::mat4 Scene::CalculateEntityTransform(Entity target, glm::mat4* outParentTransform)
    {
        if (outParentTransform)
            *outParentTransform = glm::mat4(1.f);
        if (!target.HasComponent<TransformComponent>())
            return glm::mat4(1.f);

        glm::mat4 parentTransform = GetEntityParentTransform(target);
        if (outParentTransform)
            *outParentTransform = parentTransform;
        return parentTransform * target.GetComponent<TransformComponent>().GetTransformMatrix();
    }

    glm::mat4 Scene::GetEntityParentTransform(Entity target)
    {
        if (target.HasComponent<ParentComponent>())
            return CalculateEntityTransform(GetEntityFromUUID(target.GetComponent<ParentComponent>().ParentUUID));

        return glm::mat4(1.f);
    }

    void Scene::ClearScene()
    {
        m_Registry.clear();
    }

    Entity Scene::GetEntityFromUUID(UUID uuid)
    {
        return { this, m_UUIDMap[uuid] };
    }
    
    const glm::mat4& Scene::GetEntityCachedTransform(Entity entity)
    {
        return m_CachedTransforms[entity.GetHandle()];
    }

    void Scene::CacheEntityTransform(Entity entity, bool propagateToChildren)
    {
        m_CachedTransforms[entity.GetHandle()] = CalculateEntityTransform(entity);

        if (propagateToChildren && entity.HasComponent<ChildComponent>())
        {
            auto& childComp = entity.GetComponent<ChildComponent>();

            for (auto& child : childComp.Children)
                CacheEntityTransform(GetEntityFromUUID(child));
        }
    }
}