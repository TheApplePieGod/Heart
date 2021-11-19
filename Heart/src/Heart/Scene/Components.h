#pragma once

#include "Heart/Core/UUID.h"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"

namespace Heart
{
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
    };

    struct MeshComponent
    {
        UUID Mesh;
        std::vector<UUID> Materials;
    };
}