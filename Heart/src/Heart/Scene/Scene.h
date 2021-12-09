#pragma once

#include "Heart/Renderer/EnvironmentMap.h"
#include "Heart/Core/UUID.h"
#include "entt/entt.hpp"
#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"

namespace Heart
{
    class Entity;
    class Scene
    {
    public:
        Scene();
        ~Scene();

        Entity CreateEntity(const std::string& name);
        Entity CreateEntityWithUUID(const std::string& name, UUID uuid);
        Entity DuplicateEntity(Entity source, bool keepParent, bool keepChildren);
        void DestroyEntity(Entity entity);
        void AssignRelationship(Entity parent, Entity child);
        void UnparentEntity(Entity child);

        glm::mat4 CalculateEntityTransform(Entity target, glm::mat4* outParentTransform = nullptr);
        glm::mat4 GetEntityParentTransform(Entity target);
        const glm::mat4& GetEntityCachedTransform(Entity entity);
        glm::vec3 GetEntityCachedPosition(Entity entity);
        glm::vec3 GetEntityCachedRotation(Entity entity);
        glm::vec3 GetEntityCachedScale(Entity entity);
        void CacheEntityTransform(Entity entity, bool propagateToChildren = true);

        template<typename Component>
        void ClearComponent()
        {
            m_Registry.clear<Component>();
        }

        void ClearScene();
        void SetEnvironmentMap(UUID mapAsset);

        inline entt::registry& GetRegistry() { return m_Registry; }
        inline EnvironmentMap* GetEnvironmentMap() { return m_EnvironmentMap.get(); }
        Entity GetEntityFromUUID(UUID uuid);

    private:
        struct CachedTransform
        {
            glm::mat4 Transform;
            glm::vec3 Position;
            glm::vec3 Rotation;
            glm::vec3 Scale;
        };

    private:
        template<typename Component>
        void CopyComponent(entt::entity src, entt::entity dst)
        {
            if (m_Registry.any_of<Component>(src))
                m_Registry.emplace<Component>(dst, m_Registry.get<Component>(src));
        }

        void RemoveChild(UUID parentUUID, UUID childUUID);
        void DestroyChildren(Entity parent);

    private:
        entt::registry m_Registry;
        std::unordered_map<UUID, entt::entity> m_UUIDMap;
        std::unordered_map<entt::entity, CachedTransform> m_CachedTransforms;
        Ref<EnvironmentMap> m_EnvironmentMap;

        friend class Entity;
    };
}