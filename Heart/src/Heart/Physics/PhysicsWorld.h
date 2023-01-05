#pragma once

#include "Heart/Physics/PhysicsBody.h"
#include "Heart/Core/UUID.h"
#include "glm/vec3.hpp"

class btSequentialImpulseConstraintSolver;
class btDefaultCollisionConfiguration;
class btDiscreteDynamicsWorld;
class btCollisionDispatcher;
class btBroadphaseInterface;
namespace Heart
{
    struct RaycastResult
    {
        glm::vec3 HitLocation;
        glm::vec3 HitNormal;
        float HitFraction;
        UUID HitEntityId;
    };

    struct RaycastInfo
    {
        u32 TraceChannels = (u32)DefaultCollisionChannel::Default;
        u32 TraceMask = (u32)DefaultCollisionChannel::All;
        glm::vec3 Start;
        glm::vec3 End;
        bool DrawDebugLine = false;
    };

    class Entity;
    class PhysicsWorld
    {
    public:
        PhysicsWorld() = default;
        PhysicsWorld(
            glm::vec3 gravity,
            std::function<void(UUID, UUID)> collisionStartCallback,
            std::function<void(UUID, UUID)> collisionEndCallback
        );
        ~PhysicsWorld();
        
        void Step(float stepSeconds);
        bool RaycastSingle(const RaycastInfo& info, RaycastResult& outResult);
        std::vector<RaycastResult> RaycastMulti();
        
        u32 AddBody(const PhysicsBody& body);
        PhysicsBody* GetBody(u32 id);
        void RemoveBody(u32 id);
        void ReplaceBody(u32 id, const PhysicsBody& newBody, bool keepVel = false);
        
        void SetGravity(glm::vec3 gravity);
        glm::vec3 GetGravity();
        
        inline btDiscreteDynamicsWorld* GetWorld() const { return m_World.get(); }
        inline const auto& GetCollisionStartCallback() const { return m_CollisionStartCallback; }
        inline const auto& GetCollisionEndCallback() const { return m_CollisionEndCallback; }
        
    public:
        static void Initialize();
        
        static PhysicsWorld* GetProcessingWorld() { return s_ProcessingWorld; }
        
    private:
        Ref<btSequentialImpulseConstraintSolver> m_Solver;
        Ref<btDefaultCollisionConfiguration> m_CollisionConfig;
        Ref<btDiscreteDynamicsWorld> m_World;
        Ref<btCollisionDispatcher> m_Dispatcher;
        Ref<btBroadphaseInterface> m_BroadInterface;

        u32 m_BodyIdCounter = 0;
        std::unordered_map<u32, PhysicsBody> m_Bodies;
        std::function<void(UUID, UUID)> m_CollisionStartCallback;
        std::function<void(UUID, UUID)> m_CollisionEndCallback;
        
    private:
        inline static PhysicsWorld* s_ProcessingWorld = nullptr;
    };
}
