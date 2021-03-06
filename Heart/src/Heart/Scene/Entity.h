#pragma once

#include "Heart/Core/UUID.h"
#include "entt/entt.hpp"
#include "Heart/Scene/Components.h"
#include "Heart/Scene/Scene.h"

namespace Heart
{
    class Entity
    {
    public:
        Entity(Scene* scene, entt::entity handle);
        Entity(Scene* scene, u32 handle);
        Entity() = default;

        template<typename Component>
        bool HasComponent()
        {
            return m_Scene->GetRegistry().all_of<Component>(m_EntityHandle);
        }

        // Will replace existing component of same type
        template<typename Component, typename ... Args>
        Component& AddComponent(Args&& ... args)
        {
            return m_Scene->GetRegistry().emplace_or_replace<Component>(m_EntityHandle, std::forward<Args>(args)...);
        }

        // Safe to call when entity does not have component
        template<typename Component>
        void RemoveComponent()
        {
            m_Scene->GetRegistry().remove<Component>(m_EntityHandle);
        }

        template<typename Component>
        Component& GetComponent()
        {
            HE_ENGINE_ASSERT(HasComponent<Component>(), "Cannot get, entity does not have component");
            return m_Scene->GetRegistry().get<Component>(m_EntityHandle);
        }

        bool IsValid();
        void Destroy();
        
        inline Scene* GetScene() { return m_Scene; }
        inline entt::entity GetHandle() const { return m_EntityHandle; }
        inline glm::vec3 GetPosition() { return GetComponent<TransformComponent>().Translation; }
        inline glm::vec3 GetRotation() { return GetComponent<TransformComponent>().Rotation; }
        inline glm::vec3 GetScale() { return GetComponent<TransformComponent>().Scale; }
        inline glm::vec3 GetForwardVector() { return GetComponent<TransformComponent>().GetForwardVector(); }
        inline glm::mat4x4 GetTransformMatrix() { return GetComponent<TransformComponent>().GetTransformMatrix(); }
        inline UUID GetUUID() { return GetComponent<IdComponent>().UUID; }
        inline const std::string& GetName() { return GetComponent<NameComponent>().Name; }

        glm::vec3 GetWorldPosition();
        glm::vec3 GetWorldRotation();
        glm::vec3 GetWorldScale();
        glm::vec3 GetWorldForwardVector();
        const glm::mat4x4& GetWorldTransformMatrix();

        void SetPosition(glm::vec3 pos);
        void SetRotation(glm::vec3 rot);
        void SetScale(glm::vec3 scale);
        void SetTransform(glm::vec3 pos, glm::vec3 rot, glm::vec3 scale);

    private:
        entt::entity m_EntityHandle = entt::null;
        Scene* m_Scene = nullptr;
    };
}