#include "sloth_camera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace sloth {
    void Camera::SetRotation( f32 newYawDegrees, f32 newPitchDegrees ) {
        yawDegrees = newYawDegrees;
        pitchDegrees = newPitchDegrees;
        RecalculateView();
    }

    void Camera::SetPerspective( f32 newFovYDegrees, f32 newAspectRatio, f32 newNearClip, f32 newFarClip ) {
        fovYDegrees = newFovYDegrees;
        aspectRatio = newAspectRatio;
        nearClip = newNearClip;
        farClip = newFarClip;
        RecalculateProjection();
    }

    void Camera::SetAspectRatio( f32 newAspectRatio ) {
        aspectRatio = newAspectRatio;
        RecalculateProjection();
    }

    void Camera::RecalculateView() {
        f32 yawRadians = glm::radians( yawDegrees );
        f32 pitchRadians = glm::radians( pitchDegrees );

        forward = glm::normalize( glm::vec3(
            glm::cos( yawRadians ) * glm::cos( pitchRadians ),
            glm::sin( pitchRadians ),
            glm::sin( yawRadians ) * glm::cos( pitchRadians ) ) );

        static const glm::vec3 worldUp( 0.0f, 1.0f, 0.0f );
        right = glm::normalize( glm::cross( forward, worldUp ) );
        up = glm::normalize( glm::cross( right, forward ) );

        viewMatrix = glm::lookAt( position, position + forward, up );
    }

    void Camera::RecalculateProjection() {
        projectionMatrix = glm::perspective( glm::radians( fovYDegrees ), aspectRatio, nearClip, farClip );
    }

    void Camera::ScreenPointToRay( f32 screenX, f32 screenY, f32 screenWidth, f32 screenHeight, glm::vec3 & outOrigin, glm::vec3 & outDirection ) const {
        glm::vec4 viewport( 0.0f, 0.0f, screenWidth, screenHeight );
        f32 windowY = screenHeight - screenY;

        glm::vec3 nearPoint = glm::unProject( glm::vec3( screenX, windowY, 0.0f ), viewMatrix, projectionMatrix, viewport );
        glm::vec3 farPoint = glm::unProject( glm::vec3( screenX, windowY, 1.0f ), viewMatrix, projectionMatrix, viewport );

        outOrigin = nearPoint;
        outDirection = glm::normalize( farPoint - nearPoint );
    }
} // namespace sloth
