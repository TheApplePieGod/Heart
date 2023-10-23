#pragma once

#include "Heart/Core/UUID.h"
#include "entt/entt.hpp"
#include "Heart/Scene/Components.h"
#include "Heart/Scene/Scene.h"

namespace Heart
{
    class HString;
    class Variant;
    class Entity
    {
    public:
        Entity(Scene* scene, entt::entity handle);
        Entity(Scene* scene, u32 handle);
        Entity() = default;

        bool IsValid();
        void Destroy();
        
        void SetName(HStringView8 name);
        
        inline Scene* GetScene() const { return m_Scene; }
        inline entt::entity GetHandle() const { return m_EntityHandle; }
        inline glm::vec3 GetPosition() const { return GetComponent<TransformComponent>().Translation; }
        inline glm::vec3 GetRotation() const { return GetComponent<TransformComponent>().Rotation; }
        inline glm::vec3 GetScale() const { return GetComponent<TransformComponent>().Scale; }
        inline glm::vec3 GetForwardVector() const { return GetComponent<TransformComponent>().GetForwardVector(); }
        inline glm::mat4x4 GetTransformMatrix() const { return GetComponent<TransformComponent>().GetTransformMatrix(); }
        inline UUID GetUUID() const { return GetComponent<IdComponent>().UUID; }
        inline const HString& GetName() const { return GetComponent<NameComponent>().Name; }

        glm::vec3 GetWorldPosition();
        glm::vec3 GetWorldRotation();
        glm::vec3 GetWorldScale();
        glm::vec3 GetWorldForwardVector();
        const glm::mat4x4& GetWorldTransformMatrix();

        void SetPosition(glm::vec3 pos, bool cache = true);
        void SetRotation(glm::vec3 rot, bool cache = true);
        void SetScale(glm::vec3 scale, bool cache = true);
        void SetTransform(glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, bool cache = true);
        void ApplyRotation(glm::vec3 rot, bool cache = true);

        const HVector<UUID>& GetChildren();
        void AddChild(UUID uuid, bool cache = true);
        void RemoveChild(UUID uuid, bool cache = true);
        UUID GetParent() const;
        void SetParent(UUID uuid, bool cache = true);

        Variant GetScriptProperty(const HStringView8& name) const;
        void SetScriptProperty(const HStringView8& name, const Variant& value);

        void SetIsPrimaryCameraEntity(bool primary);

        PhysicsBody* GetPhysicsBody();
        void ReplacePhysicsBody(const PhysicsBody& body, bool keepVel = false);
        
        void SetText(HStringView8 text);

        RuntimeComponent& GetRuntimeComponent(s64 typeId) const;
        bool HasRuntimeComponent(s64 typeId) const;
        void AddRuntimeComponent(s64 typeId, uptr objectHandle = 0);
        void RemoveRuntimeComponent(s64 typeId);

    public:
        template<typename Component>
        bool HasComponent() const
        {
            return m_Scene->GetRegistry().all_of<Component>(m_EntityHandle);
        }

        // Will replace existing component of same type
        template<typename Component, typename ... Args>
        void AddComponent(Args&& ... args)
        {
            m_Scene->GetRegistry().emplace_or_replace<Component>(m_EntityHandle, std::forward<Args>(args)...);
        }
        
        template<>
        void AddComponent<CollisionComponent>(PhysicsBody& body)
        {
            if (HasComponent<CollisionComponent>())
                m_Scene->GetPhysicsWorld().RemoveBody(GetComponent<CollisionComponent>().BodyId);
            body.SetTransform(GetWorldPosition(), m_Scene->GetEntityCachedQuat(*this));
            u32 id = m_Scene->GetPhysicsWorld().AddBody(body);
            m_Scene->GetRegistry().emplace_or_replace<CollisionComponent>(m_EntityHandle, id);
        }

        // Safe to call when entity does not have component
        template<typename Component>
        void RemoveComponent()
        {
            m_Scene->GetRegistry().remove<Component>(m_EntityHandle);
        }

        template<typename Component>
        Component& GetComponent() const
        {
            HE_ENGINE_ASSERT(HasComponent<Component>(), "Cannot get, entity does not have component");
            return m_Scene->GetRegistry().get<Component>(m_EntityHandle);
        }

    private:
        entt::entity m_EntityHandle = entt::null;
        Scene* m_Scene = nullptr;
    };
}
