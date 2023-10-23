#pragma once

#include "Heart/Scripting/ScriptEntityInstance.h"
#include "Heart/Scripting/ScriptComponentInstance.h"
#include "Heart/Container/HVector.hpp"
#include "Heart/Container/HString.h"
#include "Heart/Core/UUID.h"
#include "Heart/Renderer/Mesh.h"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"

namespace Flourish
{
    class Buffer;
}

namespace Heart
{
    // -----------------------
    // Components all entities MUST have are:
    // Id, Name, Transform
    // -----------------------
    // When adding new components, make sure to update:
    // Scene::DuplicateEntity, Scene::Clone, SceneAsset::Serialize & Deserialize
    // -----------------------

    struct IdComponent
    {
        UUID UUID;
    };

    struct NameComponent
    {
        HString Name;
    };

    struct ParentComponent
    {
        UUID ParentUUID;
    };

    struct ChildrenComponent
    {
        HVector<UUID> Children;
    };

    struct TransformComponent
    {
        glm::vec3 Translation = { 0.f, 0.f, 0.f };
        glm::vec3 Rotation = { 0.f, 0.f, 0.f };
        glm::vec3 Scale = { 1.f, 1.f, 1.f };
        bool Dirty = true;
        
        inline glm::quat GetRotationQuat() const
        {
            return glm::quat(glm::radians(Rotation));
        }
        
        inline glm::mat4 GetTransformMatrix() const
        {
			return glm::translate(glm::mat4(1.0f), Translation)
				* glm::toMat4(GetRotationQuat())
				* glm::scale(glm::mat4(1.0f), Scale);
        }

        inline glm::vec3 GetForwardVector() const
        {
            return glm::normalize(glm::vec3(glm::toMat4(GetRotationQuat()) * glm::vec4(0.f, 0.f, 1.f, 1.f)));
        }
    };

    struct MeshComponent
    {
        UUID Mesh = 0;
        HVector<UUID> Materials;
    };

    struct LightComponent
    {
        enum class Type : u32
        {
            Disabled = 0,
            Directional = 1,
            Point = 2
        };
        inline static const char* TypeStrings[] = {
            "Disabled", "Directional", "Point"
        };

        // TODO: ambient & specular colors
        glm::vec4 Color = { 1.f, 1.f, 1.f, 1.f }; // intensity is stored in the alpha component of the color
        Type LightType = Type::Point;
        float Radius = 5.0f;
    };

    struct ScriptComponent
    {
        ScriptEntityInstance Instance;
    };

    struct PrimaryCameraComponent
    {};

    struct CameraComponent
    {
        f32 FOV = 70.f;
        f32 NearClipPlane = 0.1f;
        f32 FarClipPlane = 500.f;
    };

    struct CollisionComponent
    {
        u32 BodyId = 0;
    };

    struct DestroyedComponent
    {};

    // TODO: make mesh an asset
    // hard right now before asset system refactor
    struct TextComponent
    {
        UUID Font = 0;
        HString Text = "Text";
        float FontSize = 1.f;
        float LineHeight = 0.f;
        UUID Material = 0;
        
        Mesh ComputedMesh;
        bool Recomputing = false;
        
        void ClearRenderData();
        void RecomputeRenderData();
    };

    struct RuntimeComponent
    {
        ScriptComponentInstance Instance;
    };
}
