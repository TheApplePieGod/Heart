#include "htpch.h"
#include "EditorCamera.h"

#include "Heart/Input/Input.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/quaternion.hpp"

namespace HeartEditor
{
    EditorCamera::EditorCamera(f32 fov, f32 nearClip, f32 farClip, f32 aspectRatio)
        : Camera(fov, nearClip, farClip, aspectRatio)
    {
        UpdateViewMatrix();
    }

    void EditorCamera::OnUpdate(Heart::Timestep ts, bool viewportFocus, bool viewportHovered)
    {
        HE_PROFILE_FUNCTION();
        
        if (viewportFocus)
        {
            f32 moveSpeed = 2.f; // m/s
            f32 mouseScale = 0.1f;
            if (Heart::Input::IsKeyPressed(Heart::KeyCode::A))
                m_Position -= (m_RightVector * moveSpeed * static_cast<f32>(ts.StepSeconds()));
            if (Heart::Input::IsKeyPressed(Heart::KeyCode::D))
                m_Position += (m_RightVector * moveSpeed * static_cast<f32>(ts.StepSeconds()));
            if (Heart::Input::IsKeyPressed(Heart::KeyCode::W))
                m_Position += (m_ForwardVector * moveSpeed * static_cast<f32>(ts.StepSeconds()));
            if (Heart::Input::IsKeyPressed(Heart::KeyCode::S))
                m_Position -= (m_ForwardVector * moveSpeed * static_cast<f32>(ts.StepSeconds()));

            m_XRotation += mouseScale * static_cast<f32>(Heart::Input::GetMouseDeltaX());
            m_YRotation += -mouseScale * static_cast<f32>(Heart::Input::GetMouseDeltaY());
        }

        if (viewportHovered)
            m_Position += static_cast<f32>(Heart::Input::GetScrollOffsetY()) * m_ForwardVector;

        UpdateViewMatrix();
    }

    void EditorCamera::UpdateViewMatrix()
    {
        // rotate the camera's axes via a quaternion
        glm::vec3 rotation = { -m_YRotation, m_XRotation, 0.f };
        glm::quat q = glm::quat(glm::radians(rotation));
        m_RightVector = glm::rotate(q, glm::vec3(1.f, 0.f, 0.f));
        m_UpVector = glm::rotate(q, glm::vec3(0.f, 1.f, 0.f));
        m_ForwardVector = glm::rotate(q, glm::vec3(0.f, 0.f, 1.f));

        // calculate the basic view matrix
        m_ViewMatrix = glm::inverse(glm::mat4(
            m_RightVector.x, m_RightVector.y, m_RightVector.z, 0.f,
            m_UpVector.x, m_UpVector.y, m_UpVector.z, 0.f,
            m_ForwardVector.x, m_ForwardVector.y, m_ForwardVector.z, 0.f,
            m_Position.x, m_Position.y, m_Position.z, 1.f
        ));

        // invert the z axis
        m_ViewMatrix = glm::mat4(
            1.f, 0.f, 0.f, 0.f,
            0.f, 1.f, 0.f, 0.f,
            0.f, 0.f, -1.f, 0.f,
            0.f, 0.f, 0.f, 1.f
        ) * m_ViewMatrix;

        // invert the y axis for ViewMatrixInvertedY
        m_ViewMatrixInvertedY = glm::mat4(
            1.f, 0.f, 0.f, 0.f,
            0.f, -1.f, 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f,
            0.f, 0.f, 0.f, 1.f
        ) * m_ViewMatrix;
    }
}