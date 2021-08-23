#pragma once

#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"

/*
* Default coordinate system
*---------------------------
*                   
*        +Y    +Z
*         |  /
*         |/
*         ------- +X
*
*/

namespace Heart
{
    class Camera
    {
    public:
        enum class ProjectionType
        {
            Perspective = 0, Orthographic = 1, OrthographicFlat = 2
        };

    public:
        Camera(f32 fov, f32 nearClip, f32 farClip, f32 aspectRatio)
            : m_ProjectionType(ProjectionType::Perspective), m_FOV(fov), m_NearClip(nearClip), m_FarClip(farClip), m_AspectRatio(aspectRatio)
        { UpdateProjectionMatrix(); }

        Camera(ProjectionType type, f32 width, f32 height, f32 nearClip, f32 farClip, f32 aspectRatio)
            : m_ProjectionType(type), m_OrthoWidth(width), m_OrthoHeight(height), m_NearClip(nearClip), m_FarClip(farClip), m_AspectRatio(aspectRatio)
        { UpdateProjectionMatrix(); }

        ~Camera();

        inline ProjectionType GetType() const { return m_ProjectionType; }
        inline void UpdateWidth(f32 width) { m_OrthoWidth = width; UpdateProjectionMatrix(); }
        inline void UpdateHeight(f32 height) { m_OrthoHeight = height; UpdateProjectionMatrix(); }
        inline void UpdateAspectRatio(f32 ratio) { m_AspectRatio = ratio; UpdateProjectionMatrix(); }
        inline void UpdateFOV(f32 fov) { m_FOV = fov; UpdateProjectionMatrix(); }
        inline void UpdateNearClip(f32 clip) { m_NearClip = clip; UpdateProjectionMatrix(); }
        inline void UpdateFarClip(f32 clip) { m_FarClip = clip; UpdateProjectionMatrix(); }
        inline void UpdateProjectionType(ProjectionType type) { m_ProjectionType = type; UpdateProjectionMatrix(); }
        inline glm::mat4 GetProjectionMatrix() const { return m_ProjectionMatrix; }

    protected:
        void UpdateProjectionMatrix();

    protected:
        const glm::vec3 m_XAxis = { 1.f, 0.f, 0.f };
        const glm::vec3 m_YAxis = { 0.f, 1.f, 0.f };
        const glm::vec3 m_ZAxis = { 0.f, 0.f, 1.f };
        const glm::vec3 m_UpVector = m_YAxis;
        const glm::vec3 m_DefaultForwardVector = m_ZAxis;
        const glm::vec3 m_DefaultRightVector = m_XAxis;

        f32 m_FOV, m_OrthoWidth, m_OrthoHeight, m_NearClip, m_FarClip, m_AspectRatio;
        ProjectionType m_ProjectionType;
        glm::mat4 m_ProjectionMatrix;
    };
}