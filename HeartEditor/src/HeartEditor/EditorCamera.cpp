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
        
        bool middleMousePressed = Heart::Input::IsButtonPressed(Heart::ButtonCode::MiddleMouse);
        glm::vec2 mouseDelta = Heart::Input::GetMouseDelta();
        f32 stepSeconds = static_cast<f32>(ts.StepSeconds());
        f32 moveSpeed = 2.f; // m/s
        f32 panSpeed = 3.f; // m/s
        if (viewportFocus)
        {
            f32 mouseScale = 0.1f;
            if (Heart::Input::IsKeyPressed(Heart::KeyCode::A))
                m_Position -= (m_RightVector * moveSpeed * stepSeconds);
            if (Heart::Input::IsKeyPressed(Heart::KeyCode::D))
                m_Position += (m_RightVector * moveSpeed * stepSeconds);
            if (Heart::Input::IsKeyPressed(Heart::KeyCode::W))
                m_Position += (m_ForwardVector * moveSpeed * stepSeconds);
            if (Heart::Input::IsKeyPressed(Heart::KeyCode::S))
                m_Position -= (m_ForwardVector * moveSpeed * stepSeconds);

            
            if (!middleMousePressed)
            {
                m_Rotation.y += mouseScale * static_cast<f32>(mouseDelta.x);
                m_Rotation.x += mouseScale * static_cast<f32>(mouseDelta.y);
            }
        }

        if (viewportHovered)
        {
            // Relative pan
            if (middleMousePressed)
            {
                m_Position += m_RightVector * mouseDelta.x * panSpeed * stepSeconds;
                m_Position += -m_UpVector * mouseDelta.y * panSpeed * stepSeconds;
            }

            f32 scrollY = (f32)Heart::Input::GetAxisDelta(Heart::AxisCode::ScrollY);
            m_Position += scrollY * m_ForwardVector;
        }

        InternalUpdateViewMatrix();
    }
}
