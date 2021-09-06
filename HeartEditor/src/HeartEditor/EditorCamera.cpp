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

    void EditorCamera::OnUpdate(Heart::Timestep ts)
    {
        HE_PROFILE_FUNCTION();
        
        f32 moveSpeed = 2.f; // m/s
        f32 mouseScale = 0.1f;
        if (Heart::Input::IsKeyPressed(Heart::KeyCode::A))
            m_Position += (m_RightVector * moveSpeed * static_cast<f32>(ts.StepSeconds()));
        if (Heart::Input::IsKeyPressed(Heart::KeyCode::D))
            m_Position -= (m_RightVector * moveSpeed * static_cast<f32>(ts.StepSeconds()));
        if (Heart::Input::IsKeyPressed(Heart::KeyCode::W))
            m_Position += (m_ForwardVector * moveSpeed * static_cast<f32>(ts.StepSeconds()));
        if (Heart::Input::IsKeyPressed(Heart::KeyCode::S))
            m_Position -= (m_ForwardVector * moveSpeed * static_cast<f32>(ts.StepSeconds()));

        m_XRotation += mouseScale * static_cast<f32>(Heart::Input::GetMouseDeltaX());
        m_YRotation += -mouseScale * static_cast<f32>(Heart::Input::GetMouseDeltaY());

        UpdateViewMatrix();
    }

    void EditorCamera::UpdateViewMatrix()
    {
        glm::quat q = glm::angleAxis(glm::radians(m_XRotation), m_YAxis);
        q *= glm::angleAxis(glm::radians(-m_YRotation), m_XAxis);

        glm::vec4 forward = { m_DefaultForwardVector, 1.f };
        glm::vec3 lookAtVector = glm::mat4_cast(q) * forward;

        m_ForwardVector = lookAtVector;
        m_RightVector = glm::cross(m_ForwardVector, m_UpVector);

        m_ViewMatrix = glm::lookAt(m_Position, m_Position + lookAtVector, m_UpVector);
        //m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_ZAxis, m_UpVector);
    }
}