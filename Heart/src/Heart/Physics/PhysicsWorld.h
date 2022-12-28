#pragma once

#include "Heart/Physics/PhysicsBody.h"
#include "glm/vec3.hpp"

class btSequentialImpulseConstraintSolver;
class btDefaultCollisionConfiguration;
class btDiscreteDynamicsWorld;
class btCollisionDispatcher;
class btBroadphaseInterface;
namespace Heart
{
    class PhysicsWorld
    {
    public:
        PhysicsWorld(glm::vec3 gravity);
        
        void Step(float stepSeconds);
        
        u32 AddBody(const PhysicsBody& body);
        PhysicsBody* GetBody(u32 id);
        void RemoveBody(u32 id);
        void ReplaceBody(u32 id, const PhysicsBody& newBody, bool keepVel = false);
        
        void SetGravity(glm::vec3 gravity);
        
        inline btDiscreteDynamicsWorld* GetWorld() const { return m_World.get(); }
        
    private:
        Ref<btSequentialImpulseConstraintSolver> m_Solver;
        Ref<btDefaultCollisionConfiguration> m_CollisionConfig;
        Ref<btDiscreteDynamicsWorld> m_World;
        Ref<btCollisionDispatcher> m_Dispatcher;
        Ref<btBroadphaseInterface> m_BroadInterface;

        u32 m_BodyIdCounter = 0;
        std::unordered_map<u32, PhysicsBody> m_Bodies;
    };
}
