#pragma once

#include "Heart/Container/HString8.h"
#include "glm/vec3.hpp"
#include "glm/gtx/quaternion.hpp"

class btCollisionShape;
class btDefaultMotionState;
class btCollisionObject;
class btTransform;
namespace Heart
{
    enum class DefaultCollisionChannel : u32
    {
        All = (u32)-1,
        Default = HE_BIT(0),
        Static = HE_BIT(1),
        Dynamic = HE_BIT(2)
    };

    enum class PhysicsBodyType : u32
    {
        None = 0,
        Rigid, Ghost
    };

    enum class PhysicsBodyShape : u32
    {
        None = 0,
        Box, Sphere, Capsule
    };

    struct PhysicsBodyCreateInfo
    {
        PhysicsBodyType Type = PhysicsBodyType::Rigid;
        float Mass = 1.f;
        u32 CollisionChannels = (u32)DefaultCollisionChannel::Default;
        u32 CollisionMask = (u32)DefaultCollisionChannel::All;
        void* ExtraData = nullptr;
    };

    class PhysicsBody
    {
    public:
        PhysicsBody() = default;
        
        PhysicsBody Clone(const PhysicsBodyCreateInfo* updatedInfo = nullptr);
        glm::vec3 GetPosition();
        glm::quat GetRotation();
        void SetPosition(glm::vec3 pos, bool resetVel = true);
        void SetRotation(glm::quat rot, bool resetVel = true);
        void SetTransform(glm::vec3 pos, glm::quat rot, bool resetVel = true);
        glm::vec3 GetLinearVelocity();
        glm::vec3 GetAngularVelocity();
        void SetLinearVelocity(glm::vec3 vel);
        void SetAngularVelocity(glm::vec3 vel);
        
        inline bool IsInitialized() const { return m_Initialized; }
        inline btCollisionObject* GetBody() const { return m_Body.get(); }
        inline PhysicsBodyShape GetShapeType() const { return m_ShapeType; }
        inline PhysicsBodyType GetBodyType() const { return m_Info.Type; }
        inline float GetMass() const { return m_Info.Mass; }
        inline u32 GetCollisionChannels() const { return m_Info.CollisionChannels; }
        inline u32 GetCollisionMask() const { return m_Info.CollisionMask; }
        inline const PhysicsBodyCreateInfo& GetInfo() const { return m_Info; }
        
        // Box shape
        glm::vec3 GetBoxExtent();
        
        // Sphere shape
        float GetSphereRadius();
        
        // Capsule shape
        float GetCapsuleRadius();
        float GetCapsuleHeight();
        
    public:
        static PhysicsBody CreateDefaultBody(void* extraData);
        static PhysicsBody CreateBoxShape(const PhysicsBodyCreateInfo& createInfo, glm::vec3 halfExtent);
        static PhysicsBody CreateSphereShape(const PhysicsBodyCreateInfo& createInfo, float radius);
        static PhysicsBody CreateCapsuleShape(const PhysicsBodyCreateInfo& createInfo, float radius, float halfHeight);
        
        static void ClearCustomCollisionChannels() { s_CustomCollisionChannels.clear(); }
        static const auto& GetCustomCollisionChannels() { return s_CustomCollisionChannels; }
        static void AddCustomCollisionChannel(u32 mask, HStringView8 name) { s_CustomCollisionChannels[name] = mask; }
        
    private:
        void Initialize(const PhysicsBodyCreateInfo& createInfo, const btTransform& transform);
        
    private:
        Ref<btCollisionShape> m_Shape;
        Ref<btDefaultMotionState> m_MotionState;
        Ref<btCollisionObject> m_Body;
        
        bool m_Initialized = false;
        bool m_HasMotionState = false;
        PhysicsBodyShape m_ShapeType;
        PhysicsBodyCreateInfo m_Info;
        
    private:
        inline static std::unordered_map<HString8, u32> s_CustomCollisionChannels;
    };
}
