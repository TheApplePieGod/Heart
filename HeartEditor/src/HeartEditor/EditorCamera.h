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

        inline void AddRotation(glm::vec3 rotation) { m_Rotation += rotation; InternalUpdateViewMatrix(); };
        inline void SetRotation(glm::vec3 rotation) { m_Rotation = rotation; InternalUpdateViewMatrix(); }
        inline void SetPosition(glm::vec3 pos) { m_Position = pos; InternalUpdateViewMatrix(); }
        inline glm::mat4 GetViewProjectionMatrix() const { return m_ProjectionMatrix * m_ViewMatrix; }
        inline glm::vec3 GetPosition() const { return m_Position; }
        inline glm::vec3 GetRotation() const { return m_Rotation; }
        inline glm::vec3 GetForwardVector() const { return m_ForwardVector; }

    private:
        inline void InternalUpdateViewMatrix() { UpdateViewMatrix(m_Rotation, m_Position); }

    private:
        glm::vec3 m_Position = { 0.f, 0.f, 0.f };
        glm::vec3 m_Rotation = { 0.f, 0.f, 0.f };
    };
}