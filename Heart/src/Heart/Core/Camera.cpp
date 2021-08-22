#include "htpch.h"
#include "Camera.h"

#include "glm/gtc/matrix_transform.hpp"

namespace Heart
{
    Camera::~Camera()
    {

    }

    void Camera::UpdateProjectionMatrix()
    {
        switch (m_ProjectionType)
        {
            default: break;
            case ProjectionType::Perspective:
            {
                //m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), -m_AspectRatio, m_NearClip, m_FarClip);
                f32 n = m_NearClip;
                f32 f = m_FarClip;
                f32 fl = 1.f / tan(m_FOV * 0.5f);
                f32 a = m_AspectRatio;

                m_ProjectionMatrix = glm::mat4(
                    -fl / a, 0, 0, 0,
                    0, -fl, 0, 0,
                    0, 0, n / (f - n), (n * f) / (f - n),
                    0, 0, -1, 0
                );
                m_ProjectionMatrix = glm::transpose(m_ProjectionMatrix);

            } break;

            case ProjectionType::Orthographic:
            {
                m_ProjectionMatrix = glm::transpose(glm::ortho(-0.5f * m_OrthoWidth, 0.5f * m_OrthoWidth, 0.5f * m_OrthoHeight, -0.5f * m_OrthoHeight, m_NearClip, m_FarClip));
            } break;

            case ProjectionType::OrthographicFlat:
            {
                f32 l = -0.5f * m_OrthoWidth;
                f32 r = 0.5f * m_OrthoWidth;
                f32 b = 0.5f * m_OrthoHeight;
                f32 t = -0.5f * m_OrthoHeight;
                m_ProjectionMatrix = glm::mat4(
                    2.f / (r - l), 0, 0, 0,
                    0, 2.f / (t - b), 0, 0,
                    0, 0, 1.f / (m_NearClip - m_FarClip), 0,
                    (l + r) / (l - r), (t + b) / (b - t), m_NearClip / (m_NearClip - m_FarClip), 1
                );
                m_ProjectionMatrix = glm::transpose(m_ProjectionMatrix);
            } break;
        }
    }
}