#pragma once

#include "Heart/Renderer/EnvironmentMap.h"
#include "Heart/Core/Timestep.h"
#include "Heart/Core/UUID.h"
#include "Heart/Physics/PhysicsWorld.h"
#include "entt/entt.hpp"
#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"

namespace Heart
{
    class Entity;
    class HStringView8;
    class HArray;
    class ScriptComponent;
    class Scene
    {
    public:
        struct CachedTransformData
        {
            glm::mat4 Transform;
            glm::quat Quat;
            glm::vec3 Position;
            glm::vec3 Rotation;
            glm::vec3 Scale;
            glm::vec3 ForwardVec;
        };

    public:
        Scene();
        ~Scene();

        Entity CreateEntity(const HStringView8& name, bool cache = true);
        Entity CreateEntityWithUUID(const HStringView8& name, UUID uuid, bool cache = true);
        Entity DuplicateEntity(Entity source, bool keepParent, bool keepChildren);
        void DestroyEntity(Entity entity, bool forceCleanup = false);
        void AssignRelationship(Entity parent, Entity child, bool cache = true);
        void UnparentEntity(Entity child, bool cache = true);
        Entity GetEntityFromUUID(UUID uuid);
        Entity GetEntityFromName(const HStringView8& name);
        Entity GetPrimaryCameraEntity();

        void CacheEntityTransform(Entity entity, bool propagateToChildren = true, bool updatePhysics = true);
        void CalculateEntityTransform(Entity target, glm::mat4& outTransform, glm::vec3& outRotation);
        void GetEntityParentTransform(Entity target, glm::mat4& outTransform);
        const CachedTransformData& GetEntityCachedData(Entity entity);
        const glm::mat4& GetEntityCachedTransform(Entity entity);
        glm::vec3 GetEntityCachedPosition(Entity entity);
        glm::vec3 GetEntityCachedRotation(Entity entity);
        glm::quat GetEntityCachedQuat(Entity entity);
        glm::vec3 GetEntityCachedScale(Entity entity);
        glm::vec3 GetEntityCachedForwardVec(Entity entity);

        u32 GetAliveEntityCount();

        Ref<Scene> Clone();
        void ClearScene();
        void SetEnvironmentMap(UUID mapAsset);
        void StartRuntime();
        void StopRuntime();
        void OnUpdateRuntime(Timestep ts);
        void CacheDirtyTransforms();

        inline entt::registry& GetRegistry() { return m_Registry; }
        inline PhysicsWorld& GetPhysicsWorld() { return m_PhysicsWorld; }
        inline EnvironmentMap* GetEnvironmentMap() { return m_EnvironmentMap.get(); }
        inline const auto& GetCachedTransforms() const { return m_CachedTransforms; }
        inline decltype(auto) GetEntityIterator() { return m_Registry.storage<entt::entity>().each(); }
        
        template<typename Component>
        void ClearComponent()
        {
            m_Registry.clear<Component>();
        }

    private:
        
    private:
        template<typename Component>
        void CopyComponent(entt::entity src, Entity dst);

        void CleanupEntity(Entity entity);
        void RemoveChild(UUID parentUUID, UUID childUUID);
        void DestroyChildren(Entity parent);
        Entity GetEntityFromUUIDUnchecked(UUID uuid);
        
        void CollisionStartCallback(UUID id0, UUID id1);
        void CollisionEndCallback(UUID id0, UUID id1);
        
    private:
        entt::registry m_Registry;
        std::unordered_map<UUID, entt::entity> m_UUIDMap;
        std::unordered_map<entt::entity, CachedTransformData> m_CachedTransforms;
        PhysicsWorld m_PhysicsWorld;
        Ref<EnvironmentMap> m_EnvironmentMap; // TODO: move this out of scene
        bool m_IsRuntime = false;

        friend class Entity;
    };
}
