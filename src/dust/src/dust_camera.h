#pragma once

#include <core/sloth_defines.h>
#include <renderer/sloth_camera.h>

#include <glm/glm.hpp>

namespace dust {

    // Top-down RTS camera in the style of Kenshi's camera: looks down at a
    // focus point that slides along the ground plane rather than flying
    // freely. WASD pans the focus point, holding the right mouse button and
    // dragging orbits yaw/pitch around it, and the scroll wheel zooms the
    // distance in/out. Drives an owned sloth::Camera, whose matrices are
    // what the renderer actually consumes.
    class DustCamera {
    public:
        void                    Update(f32 deltaTime);

        sloth::Camera&          GetCamera() { return camera; }
        const sloth::Camera&    GetCamera() const { return camera; }

        void                    SetFocusPoint(const glm::vec3& newFocusPoint) { focusPoint = newFocusPoint; RecalculatePosition(); }
        const glm::vec3&        GetFocusPoint() const { return focusPoint; }

        void                    SetDistance(f32 newDistance);
        f32                     GetDistance() const { return distance; }

    private:
        void                    RecalculatePosition();

    private:
        sloth::Camera camera;

        glm::vec3 focusPoint{ 0.0f, 0.0f, 0.0f };

        // Yaw/pitch in degrees, same convention as sloth::Camera (yaw -90
        // faces -Z, negative pitch looks down).
        f32 yawDegrees = -90.0f;
        f32 pitchDegrees = -55.0f;
        f32 minPitchDegrees = -85.0f; // near-vertical, almost straight down
        f32 maxPitchDegrees = -20.0f; // shallowest allowed tilt, stays "top down"

        f32 distance = 20.0f;
        f32 minDistance = 4.0f;
        f32 maxDistance = 80.0f;

        f32 panSpeed = 1.0f;         // scaled by distance so panning feels consistent at any zoom level
        f32 rotateSensitivity = 0.2f;
        f32 zoomStep = 1.1f;
    };

}
