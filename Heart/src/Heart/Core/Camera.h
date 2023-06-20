#pragma once

#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

/*
* Default coordinate system
*---------------------------
*                   
*        +Y    +Z
*         |  /
*         |/
*         ------- +X
*
*/

namespace Heart
{
    class Camera
    {
    public:
        /**
         * @brief The possible projection types for the camera.
         * 
         * Perspective: a typical 3D projection (https://www.geeksforgeeks.org/perspective-projection-and-its-types/).
         * Orthographic: a 2D projection where objects are not affected by position in the camera's FOV (https://en.wikipedia.org/wiki/Orthographic_projection).
         * OrthographicFlat: same as Orthographic except object's Z position is normalized and no longer impacts its size on the screen.
         */
        enum class ProjectionType
        {
            Perspective = 0, Orthographic = 1, OrthographicFlat = 2
        };

    public:
        /**
         * @brief Default constructor for a perspective camera.
         * 
         * @param fov The field of view.
         * @param nearClip The near clip distance.
         * @param farClip The far clip distance.
         * @param aspectRatio The aspect ratio (width / height, ideally should be that of the camera's viewport).
         */
        Camera(f32 fov, f32 nearClip, f32 farClip, f32 aspectRatio)
            : m_ProjectionType(ProjectionType::Perspective), m_FOV(fov), m_NearClip(nearClip), m_FarClip(farClip), m_AspectRatio(aspectRatio)
        { UpdateProjectionMatrix(); }

        /**
         * @brief Default constructor.
         * 
         * @param type The projection type.
         * @param width The width of the camera's view volume (orthographic only).
         * @param height The height of the camera's view volume (orthographic only).
         * @param nearClip The near clip distance.
         * @param farClip The far clip distance.
         * @param aspectRatio The aspect ratio (perspective only, width / height, ideally should be that of the camera's viewport).
         */
        Camera(ProjectionType type, f32 width, f32 height, f32 nearClip, f32 farClip, f32 aspectRatio)
            : m_ProjectionType(type), m_OrthoWidth(width), m_OrthoHeight(height), m_NearClip(nearClip), m_FarClip(farClip), m_AspectRatio(aspectRatio)
        { UpdateProjectionMatrix(); }

        /*! @brief Default destructor. */
        ~Camera();

        /**
         * @brief Generate a view matrix looking forwards from the camera's POV
         * 
         * Because the camera itself does not store a position or rotation, it must be
         * provided via params.
         * 
         * @note Calling this will automatically update the frustum data.
         * 
         * @param position The world position of the camera.
         * @param rotation The rotation of the camera in degrees.
         */
        void UpdateViewMatrix(glm::vec3 position, glm::vec3 rotation);

        /**
         * @brief Generate a view matrix orbiting around a point.
         * 
         * Because the camera itself does not store a position or rotation, it must be
         * provided via params.
         * 
         * @note Calling this will automatically update the frustum data.
         * 
         * @param centerPoint The world space coordinates of the origin to orbit around.
         * @param radius The world space radius of the orbit.
         * @param rotation The rotation of the camera relative to the orbit in degrees.
         */
        void UpdateViewMatrix(glm::vec3 centerPoint, f32 radius, glm::vec3 rotation);

        /**
         * @brief Update the camera's view volume width (orthographic only).
         * 
         * @param width The width of the view volume.
         */
        inline void UpdateWidth(f32 width) { m_OrthoWidth = width; UpdateProjectionMatrix(); }

        /**
         * @brief Update the camera's view volume height (orthographic only).
         * 
         * @param height The height of the view volume.
         */
        inline void UpdateHeight(f32 height) { m_OrthoHeight = height; UpdateProjectionMatrix(); }

        /**
         * @brief Update the camera's aspect ratio (perspective only).
         * 
         * @param ratio The aspect ratio.
         */
        inline void UpdateAspectRatio(f32 ratio) { m_AspectRatio = ratio; UpdateProjectionMatrix(); }

        /**
         * @brief Update the camera's field of view (perspective only).
         * 
         * @param fov The field of view.
         */
        inline void UpdateFOV(f32 fov) { m_FOV = fov; UpdateProjectionMatrix(); }

        /**
         * @brief Update the camera's near clip distance.
         * 
         * @param clip The clip distance.
         */
        inline void UpdateNearClip(f32 clip) { m_NearClip = clip; UpdateProjectionMatrix(); }

        /**
         * @brief Update the camera's far clip distance.
         * 
         * @param clip The clip distance.
         */
        inline void UpdateFarClip(f32 clip) { m_FarClip = clip; UpdateProjectionMatrix(); }

        /**
         * @brief Update the camera's projection type.
         * 
         * @note This will automatically recalculate the projection matrix but NOT the frustum bounds.
         * 
         * @param type The projection type.
         */
        inline void UpdateProjectionType(ProjectionType type) { m_ProjectionType = type; UpdateProjectionMatrix(); }

        /*! @brief Get the camera's projection type. */
        inline ProjectionType GetType() const { return m_ProjectionType; }

        /*! @brief Get the camera's projection matrix. */
        inline glm::mat4 GetProjectionMatrix() const { return m_ProjectionMatrix; }

        /*! @brief Get the camera's view matrix. */
        inline glm::mat4 GetViewMatrix() const { return m_ViewMatrix; }

        /*! @brief Get the camera's view matrix except the Y axis is inverted. */
        inline glm::mat4 GetViewMatrixInvertedY() const { return m_ViewMatrixInvertedY; }

        /*! @brief Get the camera's normalized forward direction vector. */
        inline glm::vec3 GetForwardVector() const { return m_ForwardVector; }

        /*! @brief Get the camera's normalized right direction vector. */
        inline glm::vec3 GetRightVector() const { return m_RightVector; }

        /*! @brief Get the camera's normalized up direction vector. */
        inline glm::vec3 GetUpVector() const { return m_UpVector; }

        inline f32 GetNearClip() const { return m_NearClip; }
        inline f32 GetFarClip() const { return m_FarClip; }

        /**
         * @brief Get the camera's 8 world space frustum corners.
         * 
         * Ordering:
         *  Near Plane: top left, top right, bottom left, bottom right
         *  Far Plane: top left, top right, bottom left, bottom right
         */
        inline const std::array<glm::vec3, 8>& GetFrustumCorners() const { return m_FrustumCorners; }

        /**
         * @brief Get the camera's 6 frustum planes (a, b, c, -d)
         * 
         * Ordering:
         *   Left, right, top, bottom, near, far
         */
        inline const std::array<glm::vec4, 6>& GetFrustumPlanes() const { return m_FrustumPlanes; }

    protected:
        void UpdateProjectionMatrix();
        void ComputeFrustum(glm::vec3 position);

    protected:
        inline static constexpr glm::vec3 m_XAxis = { 1.f, 0.f, 0.f };
        inline static constexpr glm::vec3 m_YAxis = { 0.f, 1.f, 0.f };
        inline static constexpr glm::vec3 m_ZAxis = { 0.f, 0.f, 1.f };
        inline static constexpr glm::vec3 m_DefaultUpVector = m_YAxis;
        inline static constexpr glm::vec3 m_DefaultForwardVector = m_ZAxis;
        inline static constexpr glm::vec3 m_DefaultRightVector = m_XAxis;

        f32 m_FOV, m_OrthoWidth, m_OrthoHeight, m_NearClip, m_FarClip, m_AspectRatio;
        ProjectionType m_ProjectionType;
        glm::mat4 m_ProjectionMatrix;
        glm::mat4 m_ViewMatrix;
        glm::mat4 m_ViewMatrixInvertedY; // used for ImGuizmo
        glm::vec3 m_ForwardVector = m_DefaultForwardVector;
        glm::vec3 m_RightVector = m_DefaultRightVector;
        glm::vec3 m_UpVector = m_DefaultUpVector;

        // world space
        std::array<glm::vec3, 8> m_FrustumCorners;
        std::array<glm::vec4, 6> m_FrustumPlanes; // L R T B N F
    };
}
