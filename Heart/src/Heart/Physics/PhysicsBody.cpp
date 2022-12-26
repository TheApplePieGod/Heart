#include "hepch.h"
#include "PhysicsBody.h"

#include "btBulletDynamicsCommon.h"

namespace Heart
{
    PhysicsBody PhysicsBody::Clone()
    {
        if (!IsInitialized()) return PhysicsBody();
        
        PhysicsBody other;
        switch (GetBodyType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Unsupported body type"); }
            case PhysicsBodyType::Box:
            { other.m_Shape = CreateRef<btBoxShape>(*static_cast<btBoxShape*>(m_Shape.get())); } break;
            case PhysicsBodyType::Sphere:
            { other.m_Shape = CreateRef<btSphereShape>(*static_cast<btSphereShape*>(m_Shape.get())); } break;
        }

        other.Initialize(m_Mass);
        
        return other;
    }

    glm::vec3 PhysicsBody::GetPosition()
    {
        HE_ENGINE_ASSERT(m_Initialized);
        
        btTransform transform;
        m_MotionState->getWorldTransform(transform);
        return {
            transform.getOrigin().getX(),
            transform.getOrigin().getY(),
            transform.getOrigin().getZ(),
        };
    }

    glm::vec3 PhysicsBody::GetRotation()
    {
        HE_ENGINE_ASSERT(m_Initialized);

        btTransform transform;
        m_MotionState->getWorldTransform(transform);
        glm::vec3 rot;
        transform.getRotation().getEulerZYX(rot.z, rot.y, rot.x);
        return rot;
    }

    glm::vec3 PhysicsBody::GetExtent()
    {
        HE_ENGINE_ASSERT(m_BodyType == PhysicsBodyType::Box, "Incompatible body type");
        
        auto extent = static_cast<btBoxShape*>(m_Shape.get())->getHalfExtentsWithoutMargin();
        return { extent.x(), extent.y(), extent.z() };
    }

    float PhysicsBody::GetRadius()
    {
        HE_ENGINE_ASSERT(m_BodyType == PhysicsBodyType::Sphere, "Incompatible body type");
        
        return static_cast<btSphereShape*>(m_Shape.get())->getRadius();
    }

    PhysicsBody PhysicsBody::CreateBoxShape(float mass, glm::vec3 halfExtent)
    {
        PhysicsBody body;
        body.m_BodyType = PhysicsBodyType::Box;
        body.m_Shape = CreateRef<btBoxShape>(btVector3(
            halfExtent.x,
            halfExtent.y,
            halfExtent.z
        ));
        
        body.Initialize(mass);
        
        return body;
    }

    PhysicsBody PhysicsBody::CreateSphereShape(float mass, float radius)
    {
        PhysicsBody body;
        body.m_BodyType = PhysicsBodyType::Sphere;
        body.m_Shape = CreateRef<btSphereShape>(radius);
        
        body.Initialize(mass);
        
        return body;
    }
    
    void PhysicsBody::Initialize(float mass)
    {
        btTransform transform;
        transform.setIdentity();
        btVector3 localInertia(0, 0, 0);
        if (mass != 0)
            m_Shape->calculateLocalInertia(mass, localInertia);
        
        m_MotionState = CreateRef<btDefaultMotionState>(transform);

        btRigidBody::btRigidBodyConstructionInfo rbInfo(
            mass,
            m_MotionState.get(),
            m_Shape.get(),
            localInertia
        );
        m_Body = CreateRef<btRigidBody>(rbInfo);
        
        m_Initialized = true;
        m_Mass = mass;
    }
}
