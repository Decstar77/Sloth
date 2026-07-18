#include "FreeCamera.h"

#include <core/sloth_engine.h>

#include <algorithm>
#include <cmath>

using namespace sloth;

void FreeCamera::Update(f32 deltaTime)
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

    if (!input.IsMouseButtonDown(MouseButton::Right))
    {
        return;
    }

    f32 newYaw = camera.GetYaw() + static_cast<f32>(input.GetMouseDeltaX()) * lookSensitivity;
    f32 newPitch = camera.GetPitch() - static_cast<f32>(input.GetMouseDeltaY()) * lookSensitivity;
    newPitch = std::clamp(newPitch, -89.0f, 89.0f);
    camera.SetRotation(newYaw, newPitch);

    if (input.GetScrollDeltaY() != 0.0)
    {
        f32 factor = std::pow(scrollSpeedStep, static_cast<f32>(input.GetScrollDeltaY()));
        moveSpeed = std::clamp(moveSpeed * factor, minMoveSpeed, maxMoveSpeed);
    }

    f32 speed = moveSpeed * (input.IsKeyDown(Key::LeftShift) ? boostMultiplier : 1.0f) * deltaTime;

    glm::vec3 movement(0.0f);
    if (input.IsKeyDown(Key::W)) movement += camera.GetForward();
    if (input.IsKeyDown(Key::S)) movement -= camera.GetForward();
    if (input.IsKeyDown(Key::D)) movement += camera.GetRight();
    if (input.IsKeyDown(Key::A)) movement -= camera.GetRight();
    if (input.IsKeyDown(Key::E)) movement += glm::vec3(0.0f, 1.0f, 0.0f);
    if (input.IsKeyDown(Key::Q)) movement -= glm::vec3(0.0f, 1.0f, 0.0f);

    if (glm::length(movement) > 0.0f)
    {
        movement = glm::normalize(movement) * speed;
    }

    camera.SetPosition(camera.GetPosition() + movement);
}
