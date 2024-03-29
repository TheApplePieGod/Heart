#include "hepch.h"
#include "PhysicsWorld.h"

#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"

namespace Heart
{
    PhysicsWorld::PhysicsWorld(
        glm::vec3 gravity,
        std::function<void(UUID, UUID)> collisionStartCallback,
        std::function<void(UUID, UUID)> collisionEndCallback
    )
        : m_CollisionStartCallback(collisionStartCallback), m_CollisionEndCallback(collisionEndCallback)
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

    PhysicsWorld::~PhysicsWorld()
    {
        // Ensure objects destruct in proper order and before bodies destruct
        m_World.reset();
        m_Dispatcher.reset();
        m_BroadInterface.reset();
        m_Solver.reset();
        m_CollisionConfig.reset();
    }

	void PhysicsWorld::Step(float stepSeconds)
	{
        s_ProcessingWorld = this;
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
        
        if (body.GetBodyType() == PhysicsBodyType::Rigid)
            m_World->addRigidBody((btRigidBody*)body.GetBody(), body.GetCollisionChannels(), body.GetCollisionMask());
        else
            m_World->addCollisionObject(body.GetBody(), body.GetCollisionChannels(), body.GetCollisionMask());

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
        m_World->removeCollisionObject(body.GetBody());

		m_Bodies.erase(id);
	}

    // TODO: check setCollisionShape?
    void PhysicsWorld::ReplaceBody(u32 id, const PhysicsBody& newBody, bool keepVel)
    {
        HE_ENGINE_ASSERT(
            m_Bodies.find(id) != m_Bodies.end(),
            "ReplaceBody invalid id"
        );
        
        auto& body = m_Bodies[id];
        auto pos = body.GetPosition();
        auto rot = body.GetRotation();
        glm::vec3 linVel;
        glm::vec3 angVel;
        if (body.GetBodyType() == PhysicsBodyType::Rigid)
        {
            linVel = body.GetLinearVelocity();
            angVel = body.GetAngularVelocity();
        }
        auto usrPtr = body.GetBody()->getUserPointer();
        m_World->removeCollisionObject(body.GetBody());
        
        body = newBody;
        body.SetTransform(pos, rot);
        body.GetBody()->setUserPointer(usrPtr);
        if (body.GetBodyType() == PhysicsBodyType::Rigid)
        {
            if (keepVel)
            {
                body.SetLinearVelocity(linVel);
                body.SetAngularVelocity(angVel);
            }
            m_World->addRigidBody((btRigidBody*)body.GetBody(), body.GetCollisionChannels(), body.GetCollisionMask());
        }
        else
            m_World->addCollisionObject(body.GetBody(), body.GetCollisionChannels(), body.GetCollisionMask());
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

    void ContactStartedCallback(btPersistentManifold* const& manifold)
    {
        UUID id0 = (UUID)(intptr_t)manifold->getBody0()->getUserPointer();
        UUID id1 = (UUID)(intptr_t)manifold->getBody1()->getUserPointer();
        
        PhysicsWorld::GetProcessingWorld()->GetCollisionStartCallback()(id0, id1);
    }

    void ContactEndedCallback(btPersistentManifold* const& manifold)
    {
        UUID id0 = (UUID)(intptr_t)manifold->getBody0()->getUserPointer();
        UUID id1 = (UUID)(intptr_t)manifold->getBody1()->getUserPointer();
        
        PhysicsWorld::GetProcessingWorld()->GetCollisionEndCallback()(id0, id1);
    }

    void PhysicsWorld::Initialize()
    {
        gContactStartedCallback = ContactStartedCallback;
        gContactEndedCallback = ContactEndedCallback;

        HE_ENGINE_LOG_DEBUG("Physics world ready");
    }
}
