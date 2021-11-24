#pragma once

#include "Heart/Core/Camera.h"
#include "Heart/Core/Timestep.h"

namespace HeartEditor
{
    class EditorCamera : public Heart::Camera
    {
    public:
        EditorCamera(f32 fov, f32 nearClip, f32 farClip, f32 aspectRatio, glm::vec3 position);

        void OnUpdate(Heart::Timestep ts, bool viewportFocused, bool viewportHovered);

        inline void AddRotation(f32 x, f32 y) { m_XRotation += x; m_YRotation += y; InternalUpdateViewMatrix(); };
        inline glm::mat4 GetViewProjectionMatrix() const { return m_ProjectionMatrix * m_ViewMatrix; }
        inline glm::vec3 GetPosition() const { return m_Position; }
        inline void SetPosition(glm::vec3 pos) { m_Position = pos; InternalUpdateViewMatrix(); }
        inline f32 GetXRotation() const { return m_XRotation; }
        inline f32 GetYRotation() const { return m_YRotation; }
        inline glm::vec3 GetForwardVector() const { return m_ForwardVector; }

    private:
        inline void InternalUpdateViewMatrix() { UpdateViewMatrix(m_XRotation, m_YRotation, m_Position); }

    private:
        glm::vec3 m_Position = { 0.f, 0.f, 0.f };
        f32 m_XRotation = 0;
        f32 m_YRotation = 0;
    };
}