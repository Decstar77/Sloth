#pragma once

#include "core/sloth_defines.h"

#include <glm/glm.hpp>

namespace sloth {

    // Vertex layout used by StaticMesh: position + color, both vec3.
    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;
    };

    // A GPU vertex/index buffer pair (VAO + VBO + EBO) for immutable
    // geometry uploaded once at construction time. Not meant for geometry
    // that changes every frame.
    class StaticMesh {
      public:
        StaticMesh( const Vertex * vertices, u32 vertexCount, const u32 * indices, u32 indexCount );
        ~StaticMesh();

        SL_NON_COPYABLE( StaticMesh );
        SL_NON_MOVABLE( StaticMesh );

        void Draw() const;

      private:
        u32 vertexArray = 0;
        u32 vertexBuffer = 0;
        u32 indexBuffer = 0;
        u32 indexCount = 0;
    };

} // namespace sloth
