#pragma once

#include "glm/vec3.hpp"

class btCollisionShape;
class btDefaultMotionState;
class btRigidBody;
class btTransform;
namespace Heart
{
    enum PhysicsBodyType
    {
        None = 0,
        Box, Sphere
    };

    class PhysicsBody
    {
    public:
        PhysicsBody() = default;
        
        PhysicsBody Clone();
        glm::vec3 GetPosition();
        glm::vec3 GetRotation();
        void SetPosition(glm::vec3 pos, bool resetVel = true);
        void SetRotation(glm::vec3 rot, bool resetVel = true);
        void SetTransform(glm::vec3 pos, glm::vec3 rot, bool resetVel = true);
        
        inline bool IsInitialized() const { return m_Initialized; }
        inline btRigidBody* GetBody() const { return m_Body.get(); }
        inline PhysicsBodyType GetBodyType() const { return m_BodyType; }
        inline float GetMass() const { return m_Mass; }
        
        // Box shape
        glm::vec3 GetExtent();
        
        // Sphere shape
        float GetRadius();
        
    public:
        static PhysicsBody CreateBoxShape(float mass, glm::vec3 halfExtent);
        static PhysicsBody CreateSphereShape(float mass, float radius);
        
    private:
        void Initialize(float mass, const btTransform& transform);
        
    private:
        Ref<btCollisionShape> m_Shape;
        Ref<btDefaultMotionState> m_MotionState;
        Ref<btRigidBody> m_Body;
        
        bool m_Initialized = false;
        PhysicsBodyType m_BodyType;
        float m_Mass;
    };
}
