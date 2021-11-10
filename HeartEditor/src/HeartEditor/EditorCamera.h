#pragma once

#include "Heart/Core/Camera.h"
#include "Heart/Core/Timestep.h"

namespace HeartEditor
{
    class EditorCamera : public Heart::Camera
    {
    public:
        EditorCamera(f32 fov, f32 nearClip, f32 farClip, f32 aspectRatio);

        void OnUpdate(Heart::Timestep ts, bool viewportFocused, bool viewportHovered);

        inline glm::mat4 GetViewMatrix() const { return m_ViewMatrix; }
        inline glm::mat4 GetViewMatrixInvertedY() const { return m_ViewMatrixInvertedY; }
        inline void AddRotation(f32 x, f32 y) { m_XRotation += x; m_YRotation += y; UpdateViewMatrix(); };
        inline glm::mat4 GetViewProjectionMatrix() const { return m_ProjectionMatrix * m_ViewMatrix; }
        inline glm::vec3 GetPosition() const { return m_Position; }
        inline f32 GetXRotation() const { return m_XRotation; }
        inline f32 GetYRotation() const { return m_YRotation; }
        inline glm::vec3 GetForwardVector() const { return m_ForwardVector; }

    private:
        void UpdateViewMatrix();

    private:
        glm::mat4 m_ViewMatrix;
        glm::mat4 m_ViewMatrixInvertedY; // used for ImGuizmo
        glm::vec3 m_Position = { 0.f, 0.f, -2.f };
        glm::vec3 m_ForwardVector = m_DefaultForwardVector;
        glm::vec3 m_RightVector = m_DefaultRightVector;
        glm::vec3 m_UpVector = m_DefaultUpVector;
        f32 m_XRotation = 0;
        f32 m_YRotation = 0;
    };
}