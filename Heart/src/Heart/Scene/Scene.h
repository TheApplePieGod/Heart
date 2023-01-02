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
        Scene();
        ~Scene();

        Entity CreateEntity(const HStringView8& name);
        Entity CreateEntityWithUUID(const HStringView8& name, UUID uuid);
        Entity DuplicateEntity(Entity source, bool keepParent, bool keepChildren);
        void DestroyEntity(Entity entity, bool forceCleanup = false);
        void AssignRelationship(Entity parent, Entity child);
        void UnparentEntity(Entity child, bool recache = true);
        Entity GetEntityFromUUID(UUID uuid);
        Entity GetEntityFromName(const HStringView8& name);
        Entity GetPrimaryCameraEntity();

        void CacheEntityTransform(Entity entity, bool propagateToChildren = true, bool updatePhysics = true);
        void CalculateEntityTransform(Entity target, glm::mat4& outTransform, glm::vec3& outRotation);
        void GetEntityParentTransform(Entity target, glm::mat4& outTransform);
        const glm::mat4& GetEntityCachedTransform(Entity entity);
        glm::vec3 GetEntityCachedPosition(Entity entity);
        glm::vec3 GetEntityCachedRotation(Entity entity);
        glm::quat GetEntityCachedQuat(Entity entity);
        glm::vec3 GetEntityCachedScale(Entity entity);

        Ref<Scene> Clone();
        void ClearScene();
        void SetEnvironmentMap(UUID mapAsset);
        void StartRuntime();
        void StopRuntime();
        void OnUpdateRuntime(Timestep ts);

        inline entt::registry& GetRegistry() { return m_Registry; }
        inline PhysicsWorld& GetPhysicsWorld() { return m_PhysicsWorld; }
        inline EnvironmentMap* GetEnvironmentMap() { return m_EnvironmentMap.get(); }
        
        template<typename Component>
        void ClearComponent()
        {
            m_Registry.clear<Component>();
        }

    private:
        struct CachedTransform
        {
            glm::mat4 Transform;
            glm::quat Quat;
            glm::vec3 Position;
            glm::vec3 Rotation;
            glm::vec3 Scale;
        };
        
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
        std::unordered_map<entt::entity, CachedTransform> m_CachedTransforms;
        PhysicsWorld m_PhysicsWorld;
        Ref<EnvironmentMap> m_EnvironmentMap; // TODO: move this out of scene
        bool m_IsRuntime = false;

        friend class Entity;
    };
}
