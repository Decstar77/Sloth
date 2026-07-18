#pragma once

#include <core/sloth_defines.h>
#include <renderer/sloth_camera.h>

// A free-flying camera in the style of Unreal Engine's editor viewport
// camera: hold the right mouse button to look around and move with WASD
// (Q/E for down/up), Shift boosts speed, and the scroll wheel adjusts the
// base move speed while looking. Drives an owned sloth::Camera, whose
// matrices are what the renderer actually consumes.
class FreeCamera
{
public:
    void Update(f32 deltaTime);

    sloth::Camera& GetCamera() { return camera; }
    const sloth::Camera& GetCamera() const { return camera; }

private:
    sloth::Camera camera;

    f32 moveSpeed = 5.0f;
    f32 boostMultiplier = 4.0f;
    f32 lookSensitivity = 0.1f;
    f32 scrollSpeedStep = 1.1f;
    f32 minMoveSpeed = 0.5f;
    f32 maxMoveSpeed = 100.0f;
};
