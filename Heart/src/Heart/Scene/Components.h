#pragma once

#include "Heart/Core/UUID.h"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"

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
        std::string Name;
    };

    struct ParentComponent
    {
        UUID ParentUUID;
    };

    struct ChildComponent
    {
        std::vector<UUID> Children;
    };

    struct TransformComponent
    {
        glm::vec3 Translation = { 0.f, 0.f, 0.f };
        glm::vec3 Rotation = { 0.f, 0.f, 0.f };
        glm::vec3 Scale = { 1.f, 1.f, 1.f };

        inline glm::mat4 GetTransformMatrix() const
        {
			return glm::translate(glm::mat4(1.0f), Translation)
				* glm::toMat4(glm::quat(glm::radians(Rotation)))
				* glm::scale(glm::mat4(1.0f), Scale);
        }

        inline glm::vec3 GetForwardVector() const
        {
            return glm::normalize(glm::vec3(glm::toMat4(glm::quat(glm::radians(Rotation))) * glm::vec4(0.f, 0.f, 1.f, 1.f)));
        }
    };

    struct MeshComponent
    {
        UUID Mesh = 0;
        std::vector<UUID> Materials;
    };

    struct LightComponent
    {
        enum class Type
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
        float ConstantAttenuation = 1.0f;
        float LinearAttenuation = 0.7f;
        float QuadraticAttenuation = 1.8f;
    };

    struct ScriptComponent
    {
        std::string NamespaceName = "";
        std::string ClassName = "";
        u32 ObjectHandle = 0;

        void InstantiateObject();
        void FreeObject();
    };
}