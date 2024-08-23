#include "hepch.h"
#include "Scene.h"

#include "Heart/Core/Timing.h"
#include "Heart/Task/TaskManager.h"
#include "Heart/Task/JobManager.h"
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
            newComp.Instance.OnConstruct();
            if (m_IsRuntime)
                newComp.Instance.OnPlayStart();

            dst.AddComponent<ScriptComponent>(newComp);
        }
    }

    template <>
    void Scene::CopyComponent<RuntimeComponent>(entt::entity _src, Entity dst)
    {
        Entity src = { this, _src };
        for (auto& pair : ScriptingEngine::GetComponentClasses())
        {
            if (!src.HasRuntimeComponent(pair.first)) continue;

            ScriptComponentInstance instance(pair.first);
            instance.Instantiate();
            instance.LoadFieldsFromJson(
                src.GetRuntimeComponent(pair.first).Instance.SerializeFieldsToJson()
            );

            dst.AddRuntimeComponent(pair.first, instance.GetObjectHandle());
        }
    }

    template <>
    void Scene::CopyComponent<PrimaryCameraComponent>(entt::entity src, Entity dst)
    {
        if (m_Registry.any_of<PrimaryCameraComponent>(src))
            dst.AddComponent<PrimaryCameraComponent>();
    }

    template <>
    void Scene::CopyComponent<CollisionComponent>(entt::entity src, Entity dst)
    {
        if (m_Registry.any_of<CollisionComponent>(src))
        {
            auto& oldComp = m_Registry.get<CollisionComponent>(src);
            CollisionComponent newComp;
            newComp.BodyId = dst.GetScene()->GetPhysicsWorld().AddBody(m_PhysicsWorld.GetBody(oldComp.BodyId)->Clone());

            dst.AddComponent<CollisionComponent>(newComp);
        }
    }

    template<typename Component>
    void Scene::CopyComponent(entt::entity src, Entity dst)
    {
        if (m_Registry.any_of<Component>(src))
            dst.AddComponent<Component>(m_Registry.get<Component>(src));
    }

    Scene::Scene()
    {
        m_PhysicsWorld = PhysicsWorld(
            { 0.f, -9.8f, 0.f },
            std::bind(&Scene::CollisionStartCallback, this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&Scene::CollisionEndCallback, this, std::placeholders::_1, std::placeholders::_2)
        );
    }

    Scene::~Scene()
    {
        // Entity cleanup
        for (auto [srcHandle] : GetEntityIterator())
        {
            Entity src = { this, srcHandle };

            // Ensure all script objects have been destroyed
            if (src.HasComponent<ScriptComponent>())
                src.GetComponent<ScriptComponent>().Instance.Destroy();

            // Also destroy runtime component objects
            for (auto& pair : ScriptingEngine::GetComponentClasses())
            {
                if (!src.HasRuntimeComponent(pair.first)) continue;
                src.GetRuntimeComponent(pair.first).Instance.Destroy();
            }
        }
    }

    Entity Scene::CreateEntity(const HStringView8& name, bool cache)
    {
        return CreateEntityWithUUID(name, UUID(), cache);
    }

    Entity Scene::CreateEntityWithUUID(const HStringView8& name, UUID uuid, bool cache)
    {
        Entity entity = { this, m_Registry.create() };
        m_UUIDMap[uuid] = entity.GetHandle();

        entity.AddComponent<IdComponent>(uuid);
        entity.AddComponent<NameComponent>(name.IsEmpty() ? "New Entity" : HString(name));
        entity.AddComponent<TransformComponent>();

        // Transform component dirty by default
        if (cache)
            CacheEntityTransform(entity);
        else
            // Populate default value
            m_CachedTransforms[entity.GetHandle()] = {};

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
        CopyComponent<TransformComponent>(source.GetHandle(), newEntity);

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

        CopyComponent<MeshComponent>(source.GetHandle(), newEntity);
        CopyComponent<LightComponent>(source.GetHandle(), newEntity);
        CopyComponent<ScriptComponent>(source.GetHandle(), newEntity);
        // Do not copy primary camera component when duplicating entity
        CopyComponent<CameraComponent>(source.GetHandle(), newEntity);
        CopyComponent<CollisionComponent>(source.GetHandle(), newEntity);
        CopyComponent<TextComponent>(source.GetHandle(), newEntity);
        CopyComponent<RuntimeComponent>(source.GetHandle(), newEntity);

        CacheEntityTransform(newEntity);
        if (newEntity.HasComponent<ScriptComponent>())
            CacheDirtyTransforms();

        return newEntity;
    }

    void Scene::DestroyEntity(Entity entity, bool forceCleanup)
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
        m_UUIDMap.erase(entity.GetUUID());
        
        if (m_IsRuntime && !forceCleanup)
        {
            entity.AddComponent<DestroyedComponent>();
            entity.RemoveComponent<NameComponent>(); // To prevent entity from coming up in name search
        }
        else
            CleanupEntity(entity);
    }

    void Scene::AssignRelationship(Entity parent, Entity child, bool cache)
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

        if (cache)
            CacheEntityTransform(child);
        else
            child.GetComponent<TransformComponent>().Dirty = true;
    }

    void Scene::UnparentEntity(Entity child, bool cache)
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

        if (cache)
            CacheEntityTransform(child);
        else
            child.GetComponent<TransformComponent>().Dirty = true;
    }

    void Scene::CleanupEntity(Entity entity)
    {
        if (entity.HasComponent<CollisionComponent>())
        {
            u32 bodyId = entity.GetComponent<CollisionComponent>().BodyId;
            m_PhysicsWorld.RemoveBody(bodyId);
        }
        
        m_Registry.destroy(entity.GetHandle());
    }

    void Scene::RemoveChild(UUID parentUUID, UUID childUUID)
    {
        Entity parent = GetEntityFromUUIDUnchecked(parentUUID);

        if (parent.HasComponent<ChildrenComponent>())
        {
            auto& childComp = parent.GetComponent<ChildrenComponent>();
            for (u32 i = 0; i < childComp.Children.Count(); i++)
            {
                UUID& elem = childComp.Children[i];
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

    void Scene::CacheEntityTransform(Entity entity, bool propagateToChildren, bool updatePhysics)
    {
        HE_PROFILE_FUNCTION();

        glm::mat4 transform;
        glm::vec3 rot;
        CalculateEntityTransform(entity, transform, rot);

        glm::vec3 skew, translation, scale;
        glm::vec4 perspective;
        glm::quat quat;
    
        // Decompose the transform so we can cache the world space values of each component
        glm::decompose(transform, scale, quat, translation, skew, perspective);
                    
        m_CachedTransforms[entity.GetHandle()] = {
            transform,
            quat,
            translation,
            rot,
            scale,
            glm::normalize(glm::vec3(glm::toMat4(quat) * glm::vec4(0.f, 0.f, 1.f, 1.f)))
        };
        
        if (updatePhysics && entity.HasComponent<CollisionComponent>())
            entity.GetPhysicsBody()->SetTransform(translation, quat);

        entity.GetComponent<TransformComponent>().Dirty = false;
            
        if (propagateToChildren && entity.HasComponent<ChildrenComponent>())
        {
            auto& childComp = entity.GetComponent<ChildrenComponent>();

            for (auto& child : childComp.Children)
            {
                // Don't propagate parent position to children with rigid bodies during runtime
                // because they are meant to become disconnected
                auto childEntity = GetEntityFromUUIDUnchecked(child);
                if (m_IsRuntime && childEntity.HasComponent<CollisionComponent>()) continue;
                
                CacheEntityTransform(childEntity);
            }
        }
    }

    void Scene::CalculateEntityTransform(Entity target, glm::mat4& outTransform, glm::vec3& outRotation)
    {
        HE_PROFILE_FUNCTION();

        auto& transformComp = target.GetComponent<TransformComponent>();
        if (target.HasComponent<ParentComponent>() && (!m_IsRuntime || !target.HasComponent<CollisionComponent>()))
        {
            auto parent = GetEntityFromUUIDUnchecked(target.GetComponent<ParentComponent>().ParentUUID);
            outTransform = GetEntityCachedTransform(parent) * transformComp.GetTransformMatrix();
            outRotation = GetEntityCachedRotation(parent) + transformComp.Rotation;
            return;
        }
            
        outTransform = transformComp.GetTransformMatrix();
        outRotation = transformComp.Rotation;
    }

    void Scene::GetEntityParentTransform(Entity target, glm::mat4& outTransform)
    {
        if (target.HasComponent<ParentComponent>() && (!m_IsRuntime || !target.HasComponent<CollisionComponent>()))
        {
            glm::vec3 rot;
            CalculateEntityTransform(GetEntityFromUUIDUnchecked(target.GetComponent<ParentComponent>().ParentUUID), outTransform, rot);
            return;
        }
        outTransform = glm::mat4(1.f);
    }

    Ref<Scene> Scene::Clone()
    {
        Ref<Scene> newScene = CreateRef<Scene>();

        // Copy each entity & associated data to the new registry
        // TODO: look into speeding up / parallelization
        // potentially we could also have a mirror scene that gets edited in parallel
        // and when the user hits play it uses that one directly
        for (auto [srcHandle] : GetEntityIterator())
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
            CopyComponent<PrimaryCameraComponent>(src.GetHandle(), dst);
            CopyComponent<CameraComponent>(src.GetHandle(), dst);
            CopyComponent<CollisionComponent>(src.GetHandle(), dst);
            CopyComponent<TextComponent>(src.GetHandle(), dst);
            CopyComponent<RuntimeComponent>(src.GetHandle(), dst);
        }
        
        // Copy script component after all entities have been created
        for (auto [srcHandle] : GetEntityIterator())
        {
            Entity src = { this, srcHandle };
            Entity dst = newScene->GetEntityFromUUID(src.GetUUID());
            
            CopyComponent<ScriptComponent>(src.GetHandle(), dst);
        }

        // Ensure transform changes are reflected
        auto scriptView = m_Registry.view<ScriptComponent>();
        if (scriptView.size() > 0)
            newScene->CacheDirtyTransforms();
            
        // Copy the environment map
        newScene->m_EnvironmentMap = m_EnvironmentMap;
        
        // Copy physics settings
        newScene->GetPhysicsWorld().SetGravity(m_PhysicsWorld.GetGravity());
        
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

        HE_ENGINE_LOG_DEBUG("Starting scene runtime");

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
        auto runTimer = AggregateTimer("Scene::OnUpdateRuntime - Physics Step");
        m_PhysicsWorld.Step(ts.StepSeconds());
        runTimer.Finish();
        
        // Update positions of physics entities to reflect physics body position
        runTimer = AggregateTimer("Scene::OnUpdateRuntime - Post Physics");
        auto physView = m_Registry.view<CollisionComponent, TransformComponent>();
        JobManager::ScheduleIter(
            physView.begin(),
            physView.end(),
            [this, &physView](size_t _entity)
            {
                auto entity = (entt::entity)_entity;

                auto& bodyComp = physView.get<CollisionComponent>(entity);
                PhysicsBody* body = m_PhysicsWorld.GetBody(bodyComp.BodyId);
                
                auto& transformComp = physView.get<TransformComponent>(entity);
                
                // TODO: store body's position and check that rather than the entity's
                bool dirty = false;
                auto newPos = body->GetPosition();
                if (transformComp.Translation != newPos)
                {
                    transformComp.Translation = newPos;
                    dirty = true;
                }
                
                auto bodyRot = body->GetRotation();
                auto eq = glm::equal(m_CachedTransforms[entity].Quat, bodyRot, 0.0001f);
                if (!eq.x || !eq.y || !eq.z || !eq.w)
                {
                    transformComp.Rotation = glm::degrees(glm::eulerAngles(bodyRot));
                    dirty = true;
                }

                // Manual cache here to preserve velocities
                if (dirty)
                    CacheEntityTransform({ this, entity }, true, false);
            },
            [this, &physView](size_t _entity)
            {
                auto entity = (entt::entity)_entity;
                
                auto& bodyComp = physView.get<CollisionComponent>(entity);
                PhysicsBody* body = m_PhysicsWorld.GetBody(bodyComp.BodyId);
                
                // Static & ghost objects will never have an updated position so skip
                return body->GetBodyType() == PhysicsBodyType::Rigid && body->GetMass() != 0.f;
            }
        ).Wait();
        runTimer.Finish();
        
        // Call OnUpdate lifecycle method
        runTimer = AggregateTimer("Scene::OnUpdateRuntime - Scripts");
        auto scriptView = m_Registry.view<ScriptComponent>();
        for (auto entity : scriptView)
        {
            auto& scriptComp = scriptView.get<ScriptComponent>(entity);
            scriptComp.Instance.OnUpdate(ts);
        }
        runTimer.Finish();
        
        // Cleanup destroyed entities
        runTimer = AggregateTimer("Scene::OnUpdateRuntime - Cleanup");
        auto destroyedView = m_Registry.view<DestroyedComponent>();
        for (auto entity : destroyedView)
            CleanupEntity({ this, entity });
        runTimer.Finish();

        // Finalize transform data
        runTimer = AggregateTimer("Scene::OnUpdateRuntime - Finalize Transforms");
        CacheDirtyTransforms();
        runTimer.Finish();
    }

    Entity Scene::GetEntityFromUUID(UUID uuid)
    {
        if (m_UUIDMap.find(uuid) == m_UUIDMap.end()) return Entity();
        return GetEntityFromUUIDUnchecked(uuid);
    }
    Entity Scene::GetEntityFromName(const HStringView8& name)
    {
        auto view = m_Registry.view<NameComponent>();
        for (auto entity : view)
            if (view.get<NameComponent>(entity).Name == name)
                return { this, entity };
        return Entity();
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

    void Scene::CacheDirtyTransforms()
    {
        auto transformView = m_Registry.view<TransformComponent>();
        JobManager::ScheduleIter(
            transformView.begin(),
            transformView.end(),
            [this, &transformView](size_t _entity)
            {
                auto entity = (entt::entity)_entity;
                CacheEntityTransform({ this, entity }, true, true);
            },
            [this, &transformView](size_t _entity)
            {
                // We only want to update the highest level components that are dirty since
                // they will propagate

                Entity entity = { this, (entt::entity)_entity };

                if (!transformView.get<TransformComponent>(entity.GetHandle()).Dirty)
                    return false;

                if (entity.HasComponent<CollisionComponent>())
                    return true;

                // Stop checking if the parent has a collision component since we
                // don't follow its transform
                Entity parent = GetEntityFromUUID(entity.GetParent());
                while (parent.IsValid())
                {
                    if (transformView.get<TransformComponent>(parent.GetHandle()).Dirty)
                        return false;
                    if (parent.HasComponent<CollisionComponent>())
                        break;
                    parent = GetEntityFromUUID(parent.GetParent());
                }

                return true;
            }
        ).Wait();
    }

    void Scene::CollisionStartCallback(UUID id0, UUID id1)
    {
        auto ent0 = GetEntityFromUUIDUnchecked(id0);
        auto ent1 = GetEntityFromUUIDUnchecked(id1);
        
        if (ent0.IsValid() && ent0.HasComponent<ScriptComponent>())
            ent0.GetComponent<ScriptComponent>().Instance.OnCollisionStarted(ent1);
        if (ent1.IsValid() && ent1.HasComponent<ScriptComponent>())
            ent1.GetComponent<ScriptComponent>().Instance.OnCollisionStarted(ent0);
    }

    void Scene::CollisionEndCallback(UUID id0, UUID id1)
    {
        auto ent0 = GetEntityFromUUIDUnchecked(id0);
        auto ent1 = GetEntityFromUUIDUnchecked(id1);
        
        if (ent0.IsValid() && ent0.HasComponent<ScriptComponent>())
            ent0.GetComponent<ScriptComponent>().Instance.OnCollisionEnded(ent1);
        if (ent1.IsValid() && ent1.HasComponent<ScriptComponent>())
            ent1.GetComponent<ScriptComponent>().Instance.OnCollisionEnded(ent0);
    }

    const Scene::CachedTransformData& Scene::GetEntityCachedData(Entity entity)
    {
        return m_CachedTransforms[entity.GetHandle()];
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

    glm::quat Scene::GetEntityCachedQuat(Entity entity)
    {
        return m_CachedTransforms[entity.GetHandle()].Quat;
    }

    glm::vec3 Scene::GetEntityCachedScale(Entity entity)
    {
        return m_CachedTransforms[entity.GetHandle()].Scale;
    }

    glm::vec3 Scene::GetEntityCachedForwardVec(Entity entity)
    {
        return m_CachedTransforms[entity.GetHandle()].ForwardVec;
    }
}
