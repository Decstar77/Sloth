#include "dust_camera.h"

#include <core/sloth_engine.h>

#include <algorithm>
#include <cmath>

using namespace sloth;

namespace dust {

    void DustCamera::Update(f32 deltaTime)
    {
        Engine& engine = Engine::Get();
        Input& input = engine.GetInput();
        Window& window = engine.GetWindow();

        camera.SetAspectRatio(window.GetAspectRatio());

        if (input.IsMouseButtonPressed(MouseButton::Right))
        {
            window.SetCursorMode(CursorMode::Disabled);
        }
        else if (input.IsMouseButtonReleased(MouseButton::Right))
        {
            window.SetCursorMode(CursorMode::Normal);
        }

        if (input.IsMouseButtonDown(MouseButton::Right))
        {
            yawDegrees += static_cast<f32>(input.GetMouseDeltaX()) * rotateSensitivity;
            pitchDegrees -= static_cast<f32>(input.GetMouseDeltaY()) * rotateSensitivity;
            pitchDegrees = std::clamp(pitchDegrees, minPitchDegrees, maxPitchDegrees);
        }

        if (input.GetScrollDeltaY() != 0.0)
        {
            f32 factor = std::pow(zoomStep, static_cast<f32>(-input.GetScrollDeltaY()));
            SetDistance(distance * factor);
        }

        // Pan along the ground plane using camera-relative flat directions
        // (yaw only, no pitch) so WASD always tracks what's on screen
        // regardless of how far the camera is tilted.
        f32 yawRadians = glm::radians(yawDegrees);
        glm::vec3 flatForward = glm::normalize(glm::vec3(glm::cos(yawRadians), 0.0f, glm::sin(yawRadians)));
        glm::vec3 flatRight(-flatForward.z, 0.0f, flatForward.x);

        f32 speed = panSpeed * distance * deltaTime;

        glm::vec3 movement(0.0f);
        if (input.IsKeyDown(Key::W)) movement += flatForward;
        if (input.IsKeyDown(Key::S)) movement -= flatForward;
        if (input.IsKeyDown(Key::D)) movement += flatRight;
        if (input.IsKeyDown(Key::A)) movement -= flatRight;

        if (glm::length(movement) > 0.0f)
        {
            movement = glm::normalize(movement) * speed;
        }

        focusPoint += movement;

        RecalculatePosition();
    }

    void DustCamera::SetDistance(f32 newDistance)
    {
        distance = std::clamp(newDistance, minDistance, maxDistance);
    }

    void DustCamera::RecalculatePosition()
    {
        camera.SetRotation(yawDegrees, pitchDegrees);
        camera.SetPosition(focusPoint - camera.GetForward() * distance);
    }

}
