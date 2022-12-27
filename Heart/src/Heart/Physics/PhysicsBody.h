#pragma once

#include "glm/vec3.hpp"

class btCollisionShape;
class btDefaultMotionState;
class btRigidBody;
class btTransform;
namespace Heart
{
    class PhysicsBody
    {
    public:
        enum class Type : u32
        {
            None = 0,
            Box, Sphere, Capsule
        };
        inline static const char* TypeStrings[] = {
            "None", "Box", "Sphere", "Capsule"
        };

    public:
        PhysicsBody() = default;
        
        PhysicsBody Clone();
        glm::vec3 GetPosition();
        glm::vec3 GetRotation();
        void SetPosition(glm::vec3 pos, bool resetVel = true);
        void SetRotation(glm::vec3 rot, bool resetVel = true);
        void SetTransform(glm::vec3 pos, glm::vec3 rot, bool resetVel = true);
        glm::vec3 GetLinearVelocity();
        glm::vec3 GetAngularVelocity();
        void SetLinearVelocity(glm::vec3 vel);
        void SetAngularVelocity(glm::vec3 vel);
        
        inline bool IsInitialized() const { return m_Initialized; }
        inline btRigidBody* GetBody() const { return m_Body.get(); }
        inline Type GetBodyType() const { return m_BodyType; }
        inline float GetMass() const { return m_Mass; }
        
        // Box shape
        glm::vec3 GetBoxExtent();
        
        // Sphere shape
        float GetSphereRadius();
        
        // Capsule shape
        float GetCapsuleRadius();
        float GetCapsuleHeight();
        
    public:
        static PhysicsBody CreateBoxShape(float mass, glm::vec3 halfExtent);
        static PhysicsBody CreateSphereShape(float mass, float radius);
        static PhysicsBody CreateCapsuleShape(float mass, float radius, float halfHeight);
        
    private:
        void Initialize(float mass, const btTransform& transform);
        
    private:
        Ref<btCollisionShape> m_Shape;
        Ref<btDefaultMotionState> m_MotionState;
        Ref<btRigidBody> m_Body;
        
        bool m_Initialized = false;
        Type m_BodyType;
        float m_Mass;
    };
}
