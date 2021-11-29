#include "htpch.h"
#include "Camera.h"

#include "Heart/Renderer/Renderer.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/quaternion.hpp"

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
                f32 n = m_NearClip;
                f32 f = m_FarClip;
                f32 fl = 1.f / tan(glm::radians(m_FOV) * 0.5f);
                f32 a = m_AspectRatio;

                if (Renderer::IsUsingReverseDepth())
                {
                    m_ProjectionMatrix = glm::mat4(
                        fl / a, 0, 0, 0,
                        0, -fl, 0, 0,
                        0, 0, n / (f - n), (n * f) / (f - n),
                        0, 0, -1, 0
                    );
                }
                else
                {
                    m_ProjectionMatrix = glm::mat4(
                        fl / a, 0, 0, 0,
                        0, -fl, 0, 0,
                        0, 0, f / (n - f), (n * f) / (n - f),
                        0, 0, -1, 0
                    );
                }

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

    void Camera::UpdateViewMatrix(f32 xRotation, f32 yRotation, glm::vec3 position)
    {
        // rotate the camera's axes via a quaternion
        glm::vec3 rotation = { -yRotation, xRotation, 0.f };
        glm::quat q = glm::quat(glm::radians(rotation));
        m_RightVector = glm::rotate(q, glm::vec3(1.f, 0.f, 0.f));
        m_UpVector = glm::rotate(q, glm::vec3(0.f, 1.f, 0.f));
        m_ForwardVector = glm::rotate(q, glm::vec3(0.f, 0.f, 1.f));

        // calculate the basic view matrix
        m_ViewMatrix = glm::inverse(glm::mat4(
            m_RightVector.x, m_RightVector.y, m_RightVector.z, 0.f,
            m_UpVector.x, m_UpVector.y, m_UpVector.z, 0.f,
            m_ForwardVector.x, m_ForwardVector.y, m_ForwardVector.z, 0.f,
            position.x, position.y, position.z, 1.f
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

    void Camera::UpdateViewMatrix(glm::vec3 centerPoint, f32 radius, f32 xRotation, f32 yRotation)
    {
        // rotate the camera's axes via a quaternion
        glm::vec3 rotation = { -yRotation, xRotation, 0.f };
        glm::quat q = glm::quat(glm::radians(rotation));
        m_RightVector = glm::rotate(q, glm::vec3(1.f, 0.f, 0.f));
        m_UpVector = glm::rotate(q, glm::vec3(0.f, 1.f, 0.f));
        m_ForwardVector = glm::rotate(q, glm::vec3(0.f, 0.f, 1.f));

        glm::vec3 circularPosition = centerPoint + m_ForwardVector * radius;
        m_RightVector *= -1.f;
        m_ForwardVector *= -1.f;

        // calculate the basic view matrix
        m_ViewMatrix = glm::inverse(glm::mat4(
            m_RightVector.x, m_RightVector.y, m_RightVector.z, 0.f,
            m_UpVector.x, m_UpVector.y, m_UpVector.z, 0.f,
            m_ForwardVector.x, m_ForwardVector.y, m_ForwardVector.z, 0.f,
            circularPosition.x, circularPosition.y, circularPosition.z, 1.f
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