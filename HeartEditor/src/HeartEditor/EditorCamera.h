#pragma once

#include "Heart/Core/Camera.h"

namespace HeartEditor
{
    class EditorCamera : public Heart::Camera
    {
    public:
        EditorCamera(f32 fov, f32 nearClip, f32 farClip, f32 aspectRatio);

    private:
        void UpdateViewMatrix();

    private:
        glm::mat4 m_ViewMatrix;
        glm::vec3 m_Position;
        glm::vec3 m_Scale;
        f32 m_XRotation = 0;
        f32 m_YRotation = 0;
    };
}