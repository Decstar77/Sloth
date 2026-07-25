#pragma once

#include <core/sloth_defines.h>
#include <renderer/sloth_camera.h>

#include <glm/glm.hpp>

namespace tower {

    class TowerCamera {
      public:
        void Update( f32 deltaTime );

        sloth::Camera & GetCamera() { return camera; }
        const sloth::Camera & GetCamera() const { return camera; }

      private:
        sloth::Camera camera;
    };
}
