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

        btTransform transform;
        m_MotionState->getWorldTransform(transform);
        other.Initialize(m_Mass, transform);
        
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

    void PhysicsBody::SetPosition(glm::vec3 pos, bool resetVel)
    {
        HE_ENGINE_ASSERT(m_Initialized);
        
        btTransform transform;
        m_MotionState->getWorldTransform(transform);
        transform.setOrigin({
            pos.x,
            pos.y,
            pos.z
        });
        m_MotionState->setWorldTransform(transform);
        m_Body->setWorldTransform(transform);
        if (resetVel)
            m_Body->setLinearVelocity({ 0.f, 0.f, 0.f });
        m_Body->activate(); // Will sleep if vel is zero for too long
    }

    void PhysicsBody::SetRotation(glm::vec3 rot, bool resetVel)
    {
        HE_ENGINE_ASSERT(m_Initialized);
        
        btTransform transform;
        m_MotionState->getWorldTransform(transform);
        transform.setRotation({
            rot.z,
            rot.y,
            rot.x
        });
        m_MotionState->setWorldTransform(transform);
        m_Body->setWorldTransform(transform);
        if (resetVel)
            m_Body->setAngularVelocity({ 0.f, 0.f, 0.f });
        m_Body->activate();
    }

    void PhysicsBody::SetTransform(glm::vec3 pos, glm::vec3 rot, bool resetVel)
    {
        HE_ENGINE_ASSERT(m_Initialized);
        
        btTransform transform;
        m_MotionState->getWorldTransform(transform);
        transform.setOrigin({
            pos.x,
            pos.y,
            pos.z
        });
        transform.setRotation({
            rot.z,
            rot.y,
            rot.x
        });
        m_MotionState->setWorldTransform(transform);
        m_Body->setWorldTransform(transform);
        if (resetVel)
        {
            m_Body->setLinearVelocity({ 0.f, 0.f, 0.f });
            m_Body->setAngularVelocity({ 0.f, 0.f, 0.f });
        }
        m_Body->activate();
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
        
        btTransform transform;
        transform.setIdentity();
        body.Initialize(mass, transform);
        
        return body;
    }

    PhysicsBody PhysicsBody::CreateSphereShape(float mass, float radius)
    {
        PhysicsBody body;
        body.m_BodyType = PhysicsBodyType::Sphere;
        body.m_Shape = CreateRef<btSphereShape>(radius);
        
        btTransform transform;
        transform.setIdentity();
        body.Initialize(mass, transform);
        
        return body;
    }
    
    void PhysicsBody::Initialize(float mass, const btTransform& transform)
    {
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
