#include "hepch.h"
#include "Scene.h"

#include "Heart/Core/Timing.h"
#include "Heart/Container/HArray.h"
#include "Heart/Container/HString8.h"
#include "Heart/Scripting/ScriptingEngine.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Scene/Components.h"
#include "glm/gtx/matrix_decompose.hpp"

namespace Heart
{
    template <>
    void Scene::CopyComponent<ScriptComponent>(entt::entity src, Entity dst)
    {
        if (m_Registry.any_of<ScriptComponent>(src))
        {
            auto& oldComp = m_Registry.get<ScriptComponent>(src);
            ScriptComponent newComp = oldComp;

            // Reinstantiate a new object with copied fields
            // TODO: binary serialization will likely be faster
            newComp.Instance.ClearObjectHandle();
            newComp.Instance.Instantiate(dst);
            newComp.Instance.LoadFieldsFromJson(oldComp.Instance.SerializeFieldsToJson());
            if (m_IsRuntime)
                newComp.Instance.OnPlayStart();

            dst.AddComponent<ScriptComponent>(newComp);
        }
    }

    template <>
    void Scene::CopyComponent<PrimaryCameraComponent>(entt::entity src, Entity dst)
    {
        if (m_Registry.any_of<PrimaryCameraComponent>(src))
            dst.AddComponent<PrimaryCameraComponent>();
    }

    template <>
    void Scene::CopyComponent<RigidBodyComponent>(entt::entity src, Entity dst)
    {
        if (m_Registry.any_of<RigidBodyComponent>(src))
        {
            auto& oldComp = m_Registry.get<RigidBodyComponent>(src);
            RigidBodyComponent newComp;
            newComp.BodyId = dst.GetScene()->GetPhysicsWorld().AddBody(m_PhysicsWorld.GetBody(oldComp.BodyId)->Clone());

            dst.AddComponent<RigidBodyComponent>(newComp);
        }
    }

    template<typename Component>
    void Scene::CopyComponent(entt::entity src, Entity dst)
    {
        if (m_Registry.any_of<Component>(src))
            dst.AddComponent<Component>(m_Registry.get<Component>(src));
    }

    Scene::Scene()
        : m_PhysicsWorld({ 0.f, -9.8f, 0.f })
    {
        
    }

    Scene::~Scene()
    {
        // Ensure all script objects have been destroyed
        auto view = m_Registry.view<ScriptComponent>();
        for (auto entity : view)
        {
            auto& scriptComp = view.get<ScriptComponent>(entity);
            scriptComp.Instance.Destroy();
        }
    }

    Entity Scene::CreateEntity(const HStringView8& name)
    {
        return CreateEntityWithUUID(name, UUID());
    }

    Entity Scene::CreateEntityWithUUID(const HStringView8& name, UUID uuid)
    {
        Entity entity = { this, m_Registry.create() };
        m_UUIDMap[uuid] = entity.GetHandle();

        entity.AddComponent<IdComponent>(uuid);
        entity.AddComponent<NameComponent>(name.IsEmpty() ? "New Entity" : HStringView(name));
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
        m_Registry.emplace<NameComponent>(newEntityHandle, m_Registry.get<NameComponent>(source.GetHandle()).Name + " Copy");

        if (keepParent && source.HasComponent<ParentComponent>())
            AssignRelationship(GetEntityFromUUIDUnchecked(source.GetComponent<ParentComponent>().ParentUUID), newEntity);

        if (keepChildren && source.HasComponent<ChildrenComponent>())
        {
            auto& childComp = source.GetComponent<ChildrenComponent>();

            for (auto& child : childComp.Children)
            {
                Entity entity = DuplicateEntity(GetEntityFromUUIDUnchecked(child), false, true);
                AssignRelationship(newEntity, entity);
            }
        }

        CopyComponent<TransformComponent>(source.GetHandle(), newEntity);
        CopyComponent<MeshComponent>(source.GetHandle(), newEntity);
        CopyComponent<LightComponent>(source.GetHandle(), newEntity);
        CopyComponent<ScriptComponent>(source.GetHandle(), newEntity);
        // Do not copy primary camera component when duplicating entity
        CopyComponent<CameraComponent>(source.GetHandle(), newEntity);
        CopyComponent<RigidBodyComponent>(source.GetHandle(), newEntity);

        CacheEntityTransform(newEntity);

        return newEntity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        UnparentEntity(entity, false);
        DestroyChildren(entity);

        if (entity.HasComponent<ScriptComponent>())
        {
            auto& instance = entity.GetComponent<ScriptComponent>().Instance;
            if (m_IsRuntime)
                instance.OnPlayEnd();
            instance.Destroy();
        }

        m_CachedTransforms.erase(entity.GetHandle());
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
        if (parent.HasComponent<ChildrenComponent>())
            parent.GetComponent<ChildrenComponent>().Children.Add(childUUID);
        else
            parent.AddComponent<ChildrenComponent>(std::initializer_list<UUID>({ childUUID }));

        CacheEntityTransform(child);
    }

    void Scene::UnparentEntity(Entity child, bool recache)
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

        if (recache)
            CacheEntityTransform(child);
    }

    void Scene::RemoveChild(UUID parentUUID, UUID childUUID)
    {
        Entity parent = GetEntityFromUUIDUnchecked(parentUUID);

        if (parent.HasComponent<ChildrenComponent>())
        {
            auto& childComp = parent.GetComponent<ChildrenComponent>();
            for (UUID& elem : childComp.Children)
            {
                if (elem == childUUID)
                {
                    elem = childComp.Children.Back();
                    childComp.Children.Pop();
                    break;
                }
            }
        }
    }

    void Scene::DestroyChildren(Entity parent)
    {
        // recursively destroy children
        if (parent.HasComponent<ChildrenComponent>())
        {
            auto& childComp = parent.GetComponent<ChildrenComponent>();

            for (auto& child : childComp.Children)
            {
                Entity entity = GetEntityFromUUIDUnchecked(child);
                DestroyEntity(entity);
            }
        }
    }

    glm::mat4 Scene::CalculateEntityTransform(Entity target, glm::mat4* outParentTransform)
    {
        glm::mat4 parentTransform = GetEntityParentTransform(target);
        if (outParentTransform)
            *outParentTransform = parentTransform;
        return parentTransform * target.GetComponent<TransformComponent>().GetTransformMatrix();
    }

    glm::mat4 Scene::GetEntityParentTransform(Entity target)
    {
        if (target.HasComponent<ParentComponent>())
            return CalculateEntityTransform(GetEntityFromUUIDUnchecked(target.GetComponent<ParentComponent>().ParentUUID));

        return glm::mat4(1.f);
    }

    Ref<Scene> Scene::Clone()
    {
        Ref<Scene> newScene = CreateRef<Scene>();

        // Copy each entity & associated data to the new registry
        m_Registry.each([&](auto srcHandle)
        {
            Entity src = { this, srcHandle };
            Entity dst = { newScene.get(), newScene->GetRegistry().create() };

            UUID uuid = src.GetUUID();
            newScene->m_UUIDMap[uuid] = dst.GetHandle(); // Update dst uuid mapping
            newScene->m_CachedTransforms[dst.GetHandle()] = m_CachedTransforms[src.GetHandle()]; // Copy this entity's cached transform

            CopyComponent<IdComponent>(src.GetHandle(), dst);
            CopyComponent<NameComponent>(src.GetHandle(), dst);
            CopyComponent<ParentComponent>(src.GetHandle(), dst);
            CopyComponent<ChildrenComponent>(src.GetHandle(), dst);
            CopyComponent<TransformComponent>(src.GetHandle(), dst);
            CopyComponent<MeshComponent>(src.GetHandle(), dst);
            CopyComponent<LightComponent>(src.GetHandle(), dst);
            CopyComponent<ScriptComponent>(src.GetHandle(), dst);
            CopyComponent<PrimaryCameraComponent>(src.GetHandle(), dst);
            CopyComponent<CameraComponent>(src.GetHandle(), dst);
            CopyComponent<RigidBodyComponent>(src.GetHandle(), dst);
        });

        // Copy the environment map
        newScene->m_EnvironmentMap = m_EnvironmentMap;

        return newScene;
    }

    void Scene::SetEnvironmentMap(UUID mapAsset)
    {
        if (!mapAsset)
            m_EnvironmentMap.reset();
        else
        {
            if (m_EnvironmentMap)
                m_EnvironmentMap->UpdateMapAsset(mapAsset);
            else
                m_EnvironmentMap = CreateRef<EnvironmentMap>(mapAsset);
            m_EnvironmentMap->Recalculate();
        }
    }

    void Scene::StartRuntime()
    {
        m_IsRuntime = true;

        // Call OnPlayStart lifecycle method
        auto view = m_Registry.view<ScriptComponent>();
        for (auto entity : view)
        {
            auto& scriptComp = view.get<ScriptComponent>(entity);
            scriptComp.Instance.OnPlayStart();
        }
    }

    void Scene::StopRuntime()
    {
        m_IsRuntime = false;

        // Call OnPlayEnd lifecycle method
        auto view = m_Registry.view<ScriptComponent>();
        for (auto entity : view)
        {
            auto& scriptComp = view.get<ScriptComponent>(entity);
            scriptComp.Instance.OnPlayEnd();
        }
    }

    void Scene::OnUpdateRuntime(Timestep ts)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("Scene::OnUpdateRuntime");
        
        // Update physics
        m_PhysicsWorld.Step(ts.StepSeconds());

        // Update positions of physics entities to reflect physics body position
        auto physView = m_Registry.view<RigidBodyComponent, TransformComponent>();
        for (auto entity : physView)
        {
            // Don't use Entity::SetTransform() here because it would unnecessarily
            // try and update the physics body
            auto& bodyComp = physView.get<RigidBodyComponent>(entity);
            auto& transformComp = physView.get<TransformComponent>(entity);
            PhysicsBody* body = m_PhysicsWorld.GetBody(bodyComp.BodyId);
            transformComp.Translation = body->GetPosition();
            transformComp.Rotation = body->GetRotation();
            CacheEntityTransform({ this, entity });
        }

        // Call OnUpdate lifecycle method
        auto scriptView = m_Registry.view<ScriptComponent>();
        for (auto entity : scriptView)
        {
            auto& scriptComp = scriptView.get<ScriptComponent>(entity);
            scriptComp.Instance.OnUpdate(ts);
        }
    }

    Entity Scene::GetEntityFromUUID(UUID uuid)
    {
        if (m_UUIDMap.find(uuid) == m_UUIDMap.end()) return Entity();
        return GetEntityFromUUIDUnchecked(uuid);
    }

    Entity Scene::GetPrimaryCameraEntity()
    {
        auto view = m_Registry.view<PrimaryCameraComponent>();
        HE_ENGINE_ASSERT(view.size() <= 1, "Found more than one primary camera entity");
        if (view.size() == 1)
            return { this, view[0] };
        return Entity();
    }
    
    Entity Scene::GetEntityFromUUIDUnchecked(UUID uuid)
    {
        return { this, m_UUIDMap[uuid] };
    }

    const glm::mat4& Scene::GetEntityCachedTransform(Entity entity)
    {
        return m_CachedTransforms[entity.GetHandle()].Transform;
    }

    glm::vec3 Scene::GetEntityCachedPosition(Entity entity)
    {
        return m_CachedTransforms[entity.GetHandle()].Position;
    }

    glm::vec3 Scene::GetEntityCachedRotation(Entity entity)
    {
        return m_CachedTransforms[entity.GetHandle()].Rotation;
    }

    glm::vec3 Scene::GetEntityCachedScale(Entity entity)
    {
        return m_CachedTransforms[entity.GetHandle()].Scale;
    }

    void Scene::CacheEntityTransform(Entity entity, bool propagateToChildren)
    {
        glm::mat4 transform = CalculateEntityTransform(entity);

        glm::vec3 skew, translation, scale;
        glm::vec4 perspective;
        glm::quat rotation;
    
        // Decompose the transform so we can cache the world space values of each component
        glm::decompose(transform, scale, rotation, translation, skew, perspective);

        m_CachedTransforms[entity.GetHandle()] = {
            transform,
            translation,
            glm::degrees(glm::eulerAngles(rotation)),
            scale
        };

        if (propagateToChildren && entity.HasComponent<ChildrenComponent>())
        {
            auto& childComp = entity.GetComponent<ChildrenComponent>();

            for (auto& child : childComp.Children)
                CacheEntityTransform(GetEntityFromUUIDUnchecked(child));
        }
    }
}
