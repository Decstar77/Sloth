#pragma once

#include "core/sloth_defines.h"

#include <glm/glm.hpp>

namespace sloth {
    class Camera {
      public:
                            Camera() { RecalculateProjection(); RecalculateView(); }

        void                SetPosition( const glm::vec3& newPosition ) { position = newPosition; RecalculateView(); }
        const glm::vec3&    GetPosition() const { return position; }

        void                SetRotation( f32 newYawDegrees, f32 newPitchDegrees );  // Yaw/pitch in degrees. Yaw of -90 faces down -Z (GLM/OpenGL convention).
        f32                 GetYaw() const { return yawDegrees; }
        f32                 GetPitch() const { return pitchDegrees; }

        void                SetPerspective( f32 newFovYDegrees, f32 newAspectRatio, f32 newNearClip, f32 newFarClip );
        void                SetAspectRatio( f32 newAspectRatio );

        f32                 GetFovY() const { return fovYDegrees; }
        f32                 GetAspectRatio() const { return aspectRatio; }
        f32                 GetNearClip() const { return nearClip; }
        f32                 GetFarClip() const { return farClip; }

        const glm::vec3&    GetForward() const { return forward; }
        const glm::vec3&    GetRight() const { return right; }
        const glm::vec3&    GetUp() const { return up; }

        const glm::mat4&    GetViewMatrix() const { return viewMatrix; }
        const glm::mat4&    GetProjectionMatrix() const { return projectionMatrix; }
        glm::mat4           GetViewProjectionMatrix() const { return projectionMatrix * viewMatrix; }

      private:
        void                RecalculateView();
        void                RecalculateProjection();

      private:
        glm::vec3 position = { 0.0f, 0.0f, 0.0f };
        f32       yawDegrees = -90.0f;
        f32       pitchDegrees = 0.0f;

        f32       fovYDegrees = 60.0f;
        f32       aspectRatio = 16.0f / 9.0f;
        f32       nearClip = 0.1f;
        f32       farClip = 1000.0f;

        glm::vec3 forward { 0.0f, 0.0f, -1.0f };
        glm::vec3 right { 1.0f, 0.0f, 0.0f };
        glm::vec3 up { 0.0f, 1.0f, 0.0f };

        glm::mat4 viewMatrix { 1.0f };
        glm::mat4 projectionMatrix { 1.0f };
    };
} // namespace sloth
