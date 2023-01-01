#include "hepch.h"
#include "PhysicsWorld.h"

#include "btBulletDynamicsCommon.h"

namespace Heart
{
    PhysicsWorld::PhysicsWorld(glm::vec3 gravity)
    {
		m_CollisionConfig = CreateRef<btDefaultCollisionConfiguration>();
		m_Dispatcher = CreateRef<btCollisionDispatcher>(m_CollisionConfig.get());
		m_BroadInterface = CreateRef<btDbvtBroadphase>();
		m_Solver = CreateRef<btSequentialImpulseConstraintSolver>();
		m_World = CreateRef<btDiscreteDynamicsWorld>(
			m_Dispatcher.get(),
			m_BroadInterface.get(),
			m_Solver.get(),
			m_CollisionConfig.get()
		);
        
        SetGravity(gravity);
    }

	void PhysicsWorld::Step(float stepSeconds)
	{
        m_World->stepSimulation(stepSeconds, 10);
	}

    bool PhysicsWorld::RaycastSingle(const RaycastInfo& info, RaycastResult& outResult)
    {
        btVector3 from(info.Start.x, info.Start.y, info.Start.z);
        btVector3 to(info.End.x, info.End.y, info.End.z);

        btCollisionWorld::ClosestRayResultCallback closestResult(from, to);
        closestResult.m_collisionFilterGroup = info.TraceChannels;
        closestResult.m_collisionFilterMask = info.TraceMask;
        
        if (info.DrawDebugLine && m_World->getDebugDrawer())
            m_World->getDebugDrawer()->drawLine(from, to, btVector4(1, 0, 0, 1));
            
        m_World->rayTest(from, to, closestResult);
        if (!closestResult.hasHit()) return false;
        
        if (info.DrawDebugLine && m_World->getDebugDrawer())
            m_World->getDebugDrawer()->drawLine(from, closestResult.m_hitPointWorld, btVector4(0, 1, 0, 1));
        
        outResult.HitLocation = { closestResult.m_hitPointWorld.x(), closestResult.m_hitPointWorld.y(), closestResult.m_hitPointWorld.z() };
        outResult.HitNormal = { closestResult.m_hitNormalWorld.x(), closestResult.m_hitNormalWorld.y(), closestResult.m_hitNormalWorld.z() };
        outResult.HitFraction = closestResult.m_closestHitFraction;
        outResult.HitEntityId = (UUID)(intptr_t)closestResult.m_collisionObject->getUserPointer();
        
        return true;
    }
 
	u32 PhysicsWorld::AddBody(const PhysicsBody& body)
	{
		HE_ENGINE_ASSERT(body.IsInitialized(), "Cannot add uniitialized body to world");
		
		m_BodyIdCounter++;
		m_Bodies[m_BodyIdCounter] = body;
        
        m_World->addRigidBody(body.GetBody(), body.GetCollisionChannels(), body.GetCollisionMask());

		return m_BodyIdCounter;
	}

    PhysicsBody* PhysicsWorld::GetBody(u32 id)
	{
        HE_ENGINE_ASSERT(
			m_Bodies.find(id) != m_Bodies.end(),
			"GetBody invalid id"
		);
        
       return &m_Bodies[id];
	}

    void PhysicsWorld::RemoveBody(u32 id)
	{
        HE_ENGINE_ASSERT(
			m_Bodies.find(id) != m_Bodies.end(),
			"RemoveBody invalid id"
		);
		
		auto& body = m_Bodies[id];
        m_World->removeRigidBody(body.GetBody());

		m_Bodies.erase(id);
	}

    void PhysicsWorld::ReplaceBody(u32 id, const PhysicsBody& newBody, bool keepVel)
    {
        HE_ENGINE_ASSERT(
            m_Bodies.find(id) != m_Bodies.end(),
            "ReplaceBody invalid id"
        );
        
        auto& body = m_Bodies[id];
        auto pos = body.GetPosition();
        auto rot = body.GetRotation();
        auto linVel = body.GetLinearVelocity();
        auto angVel = body.GetAngularVelocity();
        m_World->removeRigidBody(body.GetBody());
        
        body = newBody;
        body.SetTransform(pos, rot);
        if (keepVel)
        {
            body.SetLinearVelocity(linVel);
            body.SetAngularVelocity(angVel);
        }
        m_World->addRigidBody(body.GetBody(), body.GetCollisionChannels(), body.GetCollisionMask());
    }
 
    void PhysicsWorld::SetGravity(glm::vec3 gravity)
    {
        m_World->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
    }

    glm::vec3 PhysicsWorld::GetGravity()
    {
        auto grav = m_World->getGravity();
        return { grav.x(), grav.y(), grav.z() };
    }
}
