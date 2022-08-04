#include "hepch.h"
#include "EditorCamera.h"

#include "Heart/Input/Input.h"

namespace HeartEditor
{
    EditorCamera::EditorCamera(f32 fov, f32 nearClip, f32 farClip, f32 aspectRatio, glm::vec3 position)
        : Camera(fov, nearClip, farClip, aspectRatio)
    {
        m_Position = position;
        InternalUpdateViewMatrix();
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

            m_Rotation.x += mouseScale * static_cast<f32>(Heart::Input::GetMouseDeltaX());
            m_Rotation.y += -mouseScale * static_cast<f32>(Heart::Input::GetMouseDeltaY());
        }

        if (viewportHovered)
            m_Position += static_cast<f32>(Heart::Input::GetScrollOffsetY()) * m_ForwardVector;

        InternalUpdateViewMatrix();
    }
}