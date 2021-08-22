#include "htpch.h"
#include "EditorCamera.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"

namespace HeartEditor
{
    EditorCamera::EditorCamera(f32 fov, f32 nearClip, f32 farClip, f32 aspectRatio)
        : Camera(fov, nearClip, farClip, aspectRatio)
    {
        UpdateViewMatrix();
    }

    void EditorCamera::UpdateViewMatrix()
    {
        glm::vec3 lookAtVector = m_Position;

        glm::rotate(lookAtVector, glm::radians(m_XRotation), m_XAxis);

        m_ViewMatrix = glm::lookAt(m_Position, lookAtVector, m_UpVector);
    }
}