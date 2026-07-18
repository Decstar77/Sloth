#pragma once

#include "core/sloth_defines.h"

#include <glm/glm.hpp>

namespace sloth {
    class Shader {
      public:
        Shader( const char * vertexSource, const char * fragmentSource );
        ~Shader();

        SL_NON_COPYABLE( Shader );
        SL_NON_MOVABLE( Shader );

        void Bind() const;
        void Unbind() const;

        // Binds the program before setting the uniform.
        void SetMat4( const char * name, const glm::mat4 & value );
        void SetInt( const char * name, i32 value );

        u32 GetRendererId() const { return rendererId; }

      private:
        u32 rendererId = 0;
    };

} // namespace sloth
