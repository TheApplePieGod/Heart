#include "hepch.h"
#include "PhysicsBody.h"

#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "glm/gtx/matrix_decompose.hpp"

namespace Heart
{
    PhysicsBody PhysicsBody::Clone(const PhysicsBodyCreateInfo* updatedInfo)
    {
        if (!IsInitialized()) return PhysicsBody();
        
        if (!updatedInfo) updatedInfo = &m_Info;
        
        // Should be fine to reuse shape because we never modify it directly
        PhysicsBody other;
        other.m_Shape = m_Shape;
        other.m_ShapeType = m_ShapeType;
        
        btTransform transform;
        if (m_MotionState)
            m_MotionState->getWorldTransform(transform);
        else
            transform = m_Body->getWorldTransform();
        other.Initialize(*updatedInfo, transform);
        
        return other;
    }

    glm::vec3 PhysicsBody::GetPosition()
    {
        HE_ENGINE_ASSERT(m_Initialized);
        
        btTransform transform;
        if (m_MotionState)
            m_MotionState->getWorldTransform(transform);
        else
            transform = m_Body->getWorldTransform();
        return {
            transform.getOrigin().getX(),
            transform.getOrigin().getY(),
            transform.getOrigin().getZ(),
        };
    }

    glm::quat PhysicsBody::GetRotation()
    {
        HE_ENGINE_ASSERT(m_Initialized);

        btTransform transform;
        if (m_MotionState)
            m_MotionState->getWorldTransform(transform);
        else
            transform = m_Body->getWorldTransform();
        auto rot = transform.getRotation();
        return { rot.w(), rot.x(), rot.y(), rot.z() };
    }

    void PhysicsBody::SetPosition(glm::vec3 pos, bool resetVel)
    {
        HE_ENGINE_ASSERT(m_Initialized);
        
        btTransform transform;
        if (m_MotionState)
            m_MotionState->getWorldTransform(transform);
        else
            transform = m_Body->getWorldTransform();
        transform.setOrigin({
            pos.x,
            pos.y,
            pos.z
        });
        m_Body->setWorldTransform(transform);
        
        if (resetVel && m_Info.Type == PhysicsBodyType::Rigid)
            SetLinearVelocity({ 0.f, 0.f, 0.f });
        
        if (m_MotionState)
        {
            m_MotionState->setWorldTransform(transform);
            m_Body->activate();
        }
    }

    void PhysicsBody::SetRotation(glm::quat rot, bool resetVel)
    {
        HE_ENGINE_ASSERT(m_Initialized);
        
        btTransform transform;
        if (m_MotionState)
            m_MotionState->getWorldTransform(transform);
        else
            transform = m_Body->getWorldTransform();
        transform.setRotation({
            rot.x,
            rot.y,
            rot.z,
            rot.w
        });
        m_Body->setWorldTransform(transform);
        
        if (resetVel && m_Info.Type == PhysicsBodyType::Rigid)
            SetAngularVelocity({ 0.f, 0.f, 0.f });

        if (m_MotionState)
        {
            m_MotionState->setWorldTransform(transform);
            m_Body->activate();
        }
    }

    void PhysicsBody::SetTransform(glm::vec3 pos, glm::quat rot, bool resetVel)
    {
        HE_ENGINE_ASSERT(m_Initialized);
        
        btTransform transform;
        if (m_MotionState)
            m_MotionState->getWorldTransform(transform);
        else
            transform = m_Body->getWorldTransform();
        transform.setOrigin({
            pos.x,
            pos.y,
            pos.z
        });
        transform.setRotation({
            rot.x,
            rot.y,
            rot.z,
            rot.w
        });
        m_Body->setWorldTransform(transform);
        
        if (resetVel && m_Info.Type == PhysicsBodyType::Rigid)
        {
            SetLinearVelocity({ 0.f, 0.f, 0.f });
            SetAngularVelocity({ 0.f, 0.f, 0.f });
        }
        
        if (m_MotionState)
        {
            m_MotionState->setWorldTransform(transform);
            m_Body->activate();
        }
    }

    glm::vec3 PhysicsBody::GetLinearVelocity()
    {
        HE_ENGINE_ASSERT(m_Initialized);
        HE_ENGINE_ASSERT(m_Info.Type == PhysicsBodyType::Rigid);

        auto vel = ((btRigidBody*)m_Body.get())->getLinearVelocity();
        return { vel.x(), vel.y(), vel.z() };
    }
    glm::vec3 PhysicsBody::GetAngularVelocity()
    {
        HE_ENGINE_ASSERT(m_Initialized);
        HE_ENGINE_ASSERT(m_Info.Type == PhysicsBodyType::Rigid);

        auto vel = ((btRigidBody*)m_Body.get())->getAngularVelocity();
        return { vel.x(), vel.y(), vel.z() };
    }

    void PhysicsBody::SetLinearVelocity(glm::vec3 vel)
    {
        HE_ENGINE_ASSERT(m_Initialized);
        HE_ENGINE_ASSERT(m_Info.Type == PhysicsBodyType::Rigid);
        
        ((btRigidBody*)m_Body.get())->setLinearVelocity({ vel.x, vel.y, vel.z });
    }

    void PhysicsBody::SetAngularVelocity(glm::vec3 vel)
    {
        HE_ENGINE_ASSERT(m_Initialized);
        HE_ENGINE_ASSERT(m_Info.Type == PhysicsBodyType::Rigid);
        
        ((btRigidBody*)m_Body.get())->setAngularVelocity({ vel.x, vel.y, vel.z });
    }

    glm::vec3 PhysicsBody::GetBoxExtent()
    {
        HE_ENGINE_ASSERT(m_ShapeType == PhysicsBodyShape::Box, "Incompatible body type");
        
        auto extent = static_cast<btBoxShape*>(m_Shape.get())->getHalfExtentsWithMargin();
        return { extent.x(), extent.y(), extent.z() };
    }

    float PhysicsBody::GetSphereRadius()
    {
        HE_ENGINE_ASSERT(m_ShapeType == PhysicsBodyShape::Sphere, "Incompatible body type");
        
        return static_cast<btSphereShape*>(m_Shape.get())->getRadius();
    }

    float PhysicsBody::GetCapsuleRadius()
    {
        HE_ENGINE_ASSERT(m_ShapeType == PhysicsBodyShape::Capsule, "Incompatible body type");
        
        return static_cast<btCapsuleShape*>(m_Shape.get())->getRadius();
    }

    float PhysicsBody::GetCapsuleHeight()
    {
        HE_ENGINE_ASSERT(m_ShapeType == PhysicsBodyShape::Capsule, "Incompatible body type");
        
        return static_cast<btCapsuleShape*>(m_Shape.get())->getHalfHeight();
    }

    PhysicsBody PhysicsBody::CreateDefaultBody(void* extraData)
    {
        PhysicsBodyCreateInfo info;
        info.ExtraData = extraData;
        return CreateBoxShape(info, { 0.5f, 0.5f, 0.5f });
    }

    PhysicsBody PhysicsBody::CreateBoxShape(const PhysicsBodyCreateInfo& createInfo, glm::vec3 halfExtent)
    {
        PhysicsBody body;
        body.m_ShapeType = PhysicsBodyShape::Box;
        body.m_Shape = CreateRef<btBoxShape>(btVector3(
            halfExtent.x,
            halfExtent.y,
            halfExtent.z
        ));
        
        btTransform transform;
        transform.setIdentity();
        body.Initialize(createInfo, transform);
        
        return body;
    }

    PhysicsBody PhysicsBody::CreateSphereShape(const PhysicsBodyCreateInfo& createInfo, float radius)
    {
        PhysicsBody body;
        body.m_ShapeType = PhysicsBodyShape::Sphere;
        body.m_Shape = CreateRef<btSphereShape>(radius);
        
        btTransform transform;
        transform.setIdentity();
        body.Initialize(createInfo, transform);
        
        return body;
    }

    PhysicsBody PhysicsBody::CreateCapsuleShape(const PhysicsBodyCreateInfo& createInfo, float radius, float halfHeight)
    {
        PhysicsBody body;
        body.m_ShapeType = PhysicsBodyShape::Capsule;
        body.m_Shape = CreateRef<btCapsuleShape>(radius, halfHeight * 2.f);
        
        btTransform transform;
        transform.setIdentity();
        body.Initialize(createInfo, transform);
        
        return body;
    }
    
    void PhysicsBody::Initialize(const PhysicsBodyCreateInfo& createInfo, const btTransform& transform)
    {
        switch (createInfo.Type)
        {
            default:
            { HE_ENGINE_ASSERT("Invalid body type"); } break;
            
            case PhysicsBodyType::Rigid:
            {
                m_MotionState = CreateRef<btDefaultMotionState>(transform);
                
                btVector3 localInertia(0, 0, 0);
                if (createInfo.Mass != 0)
                    m_Shape->calculateLocalInertia(createInfo.Mass, localInertia);

                btRigidBody::btRigidBodyConstructionInfo rbInfo(
                    createInfo.Mass,
                    m_MotionState.get(),
                    m_Shape.get(),
                    localInertia
                );
                m_Body = CreateRef<btRigidBody>(rbInfo);
            } break;
                
            case PhysicsBodyType::Ghost:
            {
                m_Body = CreateRef<btGhostObject>();
                m_Body->setCollisionShape(m_Shape.get());
                m_Body->setCollisionFlags(
                    m_Body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE
                );
            } break;
        }
        
        m_Body->setUserPointer(createInfo.ExtraData);
        
        m_Initialized = true;
        m_Info = createInfo;
    }
}
