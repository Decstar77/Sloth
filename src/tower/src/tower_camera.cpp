#include "tower_camera.h"

#include <core/sloth_engine.h>

#include <algorithm>
#include <cmath>

using namespace sloth;

namespace tower {

    void TowerCamera::Update( f32 deltaTime ) {
        Engine & engine = Engine::Get();
        Input & input = engine.GetInput();
        Window & window = engine.GetWindow();

        camera.SetAspectRatio( window.GetAspectRatio() );

        // Lock and hide the cursor for unbounded FPS-style mouselook - unlike
        // Dust's RTS camera, Tower's raiding gameplay has no non-look use for
        // the cursor, so it stays locked rather than toggling on a mouse
        // button.
        if ( window.GetCursorMode() != CursorMode::Disabled ) {
            window.SetCursorMode( CursorMode::Disabled );
        }

        yawDegrees += static_cast<f32>( input.GetMouseDeltaX() ) * lookSensitivity;
        pitchDegrees -= static_cast<f32>( input.GetMouseDeltaY() ) * lookSensitivity;
        pitchDegrees = std::clamp( pitchDegrees, minPitchDegrees, maxPitchDegrees );

        camera.SetRotation( yawDegrees, pitchDegrees );
    }

}
