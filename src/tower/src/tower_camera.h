#pragma once

#include <core/sloth_defines.h>
#include <renderer/sloth_camera.h>

#include <glm/glm.hpp>

namespace tower {

    // First-person look camera: mouselook only (no movement of its own -
    // whatever drives the player entity is responsible for calling
    // SetPosition() each frame, e.g. to the player capsule's eye position).
    // The cursor is locked and hidden for unbounded mouse movement, Rust/
    // Kenshi-style top-down orbiting has no place here since Tower's raiding
    // gameplay is first-person.
    class TowerCamera {
      public:
        void Update( f32 deltaTime );

        void SetPosition( const glm::vec3 & newPosition ) { camera.SetPosition( newPosition ); }

        f32 GetYaw() const { return yawDegrees; }
        f32 GetPitch() const { return pitchDegrees; }

        sloth::Camera & GetCamera() { return camera; }
        const sloth::Camera & GetCamera() const { return camera; }

      private:
        sloth::Camera camera;

        // Yaw/pitch in degrees, same convention as sloth::Camera (yaw -90
        // faces -Z, negative pitch looks down).
        f32 yawDegrees = -90.0f;
        f32 pitchDegrees = 0.0f;
        f32 minPitchDegrees = -89.0f;
        f32 maxPitchDegrees = 89.0f;

        f32 lookSensitivity = 0.15f;
    };
}
