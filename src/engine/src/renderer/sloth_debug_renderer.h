#pragma once

#include "core/sloth_defines.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <vector>

namespace sloth {

    class Shader;

    // Immediate-mode debug line drawing (wireframe spheres/boxes/cylinders
    // are built out of DrawLine segments). Lines accumulate across DrawLine
    // calls and are all drawn - then cleared - by Render(), so it's meant to
    // be fed once per frame: submit shapes during Update/Render, then call
    // Render() last with the frame's view-projection matrix.
    class DebugRenderer {
      public:
        SL_NON_COPYABLE( DebugRenderer );
        SL_NON_MOVABLE( DebugRenderer );

        static DebugRenderer & Get();

        void Init();
        void Shutdown();

        void DrawLine( const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & color );

        void DrawBox( const glm::vec3 & center, const glm::vec3 & halfExtents, const glm::quat & rotation, const glm::vec3 & color );
        void DrawSphere( const glm::vec3 & center, f32 radius, const glm::vec3 & color, u32 segments = 16 );
        void DrawCylinder( const glm::vec3 & center, f32 radius, f32 height, const glm::quat & rotation, const glm::vec3 & color, u32 segments = 16 );

        // Draws every line submitted since the last Render() call, then
        // clears them - call once per frame, after all DrawXxx calls.
        void Render( const glm::mat4 & viewProjection );

      private:
        DebugRenderer() = default;
        ~DebugRenderer() = default;

      private:
        struct LineVertex {
            glm::vec3 position;
            glm::vec3 color;
        };

        std::unique_ptr<Shader> shader;
        std::vector<LineVertex> vertices;

        u32 vertexArray = 0;
        u32 vertexBuffer = 0;
        u32 vertexBufferCapacity = 0; // In vertices.
    };

} // namespace sloth
