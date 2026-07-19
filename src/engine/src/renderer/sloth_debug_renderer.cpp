#include "sloth_debug_renderer.h"

#include "renderer/sloth_shader.h"

#include <glad/gl.h>
#include <glm/gtc/constants.hpp>

#include <cmath>

namespace sloth {

    static const char * DebugVertexShaderSource = R"(
        #version 450 core
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec3 aColor;

        uniform mat4 uViewProjection;

        out vec3 vColor;

        void main()
        {
            vColor = aColor;
            gl_Position = uViewProjection * vec4(aPosition, 1.0);
        }
    )";

    static const char * DebugFragmentShaderSource = R"(
        #version 450 core
        in vec3 vColor;
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(vColor, 1.0);
        }
    )";

    DebugRenderer & DebugRenderer::Get() {
        static DebugRenderer instance;
        return instance;
    }

    void DebugRenderer::Init() {
        SL_ASSERT_MSG( shader == nullptr, "DebugRenderer has already been initialized" );

        shader = std::make_unique<Shader>( DebugVertexShaderSource, DebugFragmentShaderSource );

        glGenVertexArrays( 1, &vertexArray );
        glBindVertexArray( vertexArray );

        glGenBuffers( 1, &vertexBuffer );
        glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );

        glEnableVertexAttribArray( 0 );
        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( LineVertex ), reinterpret_cast<void *>( offsetof( LineVertex, position ) ) );

        glEnableVertexAttribArray( 1 );
        glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof( LineVertex ), reinterpret_cast<void *>( offsetof( LineVertex, color ) ) );

        glBindVertexArray( 0 );
    }

    void DebugRenderer::Shutdown() {
        glDeleteBuffers( 1, &vertexBuffer );
        glDeleteVertexArrays( 1, &vertexArray );
        vertexBuffer = 0;
        vertexArray = 0;
        vertexBufferCapacity = 0;

        shader.reset();
        vertices.clear();
    }

    void DebugRenderer::DrawLine( const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & color ) {
        vertices.push_back( { a, color } );
        vertices.push_back( { b, color } );
    }

    void DebugRenderer::DrawBox( const glm::vec3 & center, const glm::vec3 & halfExtents, const glm::quat & rotation, const glm::vec3 & color ) {
        glm::vec3 corners[8];
        for ( u32 i = 0; i < 8; ++i ) {
            glm::vec3 signs( ( i & 1 ) ? 1.0f : -1.0f, ( i & 2 ) ? 1.0f : -1.0f, ( i & 4 ) ? 1.0f : -1.0f );
            corners[i] = center + rotation * ( signs * halfExtents );
        }

        // Bottom face (y-), top face (y+), then vertical edges connecting them.
        DrawLine( corners[0], corners[1], color );
        DrawLine( corners[1], corners[3], color );
        DrawLine( corners[3], corners[2], color );
        DrawLine( corners[2], corners[0], color );

        DrawLine( corners[4], corners[5], color );
        DrawLine( corners[5], corners[7], color );
        DrawLine( corners[7], corners[6], color );
        DrawLine( corners[6], corners[4], color );

        DrawLine( corners[0], corners[4], color );
        DrawLine( corners[1], corners[5], color );
        DrawLine( corners[2], corners[6], color );
        DrawLine( corners[3], corners[7], color );
    }

    void DebugRenderer::DrawSphere( const glm::vec3 & center, f32 radius, const glm::vec3 & color, u32 segments ) {
        segments = segments < 3 ? 3 : segments;

        // Three orthogonal circles (XY, XZ, YZ planes) - a cheap wireframe
        // sphere approximation, not a full latitude/longitude mesh.
        for ( u32 s = 0; s < segments; ++s ) {
            f32 theta0 = 2.0f * glm::pi<f32>() * static_cast<f32>( s ) / static_cast<f32>( segments );
            f32 theta1 = 2.0f * glm::pi<f32>() * static_cast<f32>( s + 1 ) / static_cast<f32>( segments );

            f32 c0 = std::cos( theta0 ), s0 = std::sin( theta0 );
            f32 c1 = std::cos( theta1 ), s1 = std::sin( theta1 );

            DrawLine( center + radius * glm::vec3( c0, s0, 0.0f ), center + radius * glm::vec3( c1, s1, 0.0f ), color );
            DrawLine( center + radius * glm::vec3( c0, 0.0f, s0 ), center + radius * glm::vec3( c1, 0.0f, s1 ), color );
            DrawLine( center + radius * glm::vec3( 0.0f, c0, s0 ), center + radius * glm::vec3( 0.0f, c1, s1 ), color );
        }
    }

    void DebugRenderer::DrawCylinder( const glm::vec3 & center, f32 radius, f32 height, const glm::quat & rotation, const glm::vec3 & color, u32 segments ) {
        segments = segments < 3 ? 3 : segments;

        f32 halfHeight = height * 0.5f;

        auto RingPosition = [&]( u32 segment, f32 y ) {
            f32 theta = 2.0f * glm::pi<f32>() * static_cast<f32>( segment ) / static_cast<f32>( segments );
            glm::vec3 local( radius * std::cos( theta ), y, radius * std::sin( theta ) );
            return center + rotation * local;
        };

        for ( u32 s = 0; s < segments; ++s ) {
            glm::vec3 top0 = RingPosition( s, halfHeight );
            glm::vec3 top1 = RingPosition( s + 1, halfHeight );
            glm::vec3 bottom0 = RingPosition( s, -halfHeight );
            glm::vec3 bottom1 = RingPosition( s + 1, -halfHeight );

            DrawLine( top0, top1, color );
            DrawLine( bottom0, bottom1, color );
            DrawLine( top0, bottom0, color );
        }
    }

    void DebugRenderer::Render( const glm::mat4 & viewProjection ) {
        if ( vertices.empty() ) {
            return;
        }

        shader->SetMat4( "uViewProjection", viewProjection );

        glBindVertexArray( vertexArray );
        glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );

        u32 vertexCount = static_cast<u32>( vertices.size() );
        if ( vertexCount > vertexBufferCapacity ) {
            glBufferData( GL_ARRAY_BUFFER, static_cast<GLsizeiptr>( sizeof( LineVertex ) * vertexCount ), vertices.data(), GL_DYNAMIC_DRAW );
            vertexBufferCapacity = vertexCount;
        } else {
            glBufferSubData( GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>( sizeof( LineVertex ) * vertexCount ), vertices.data() );
        }

        glDrawArrays( GL_LINES, 0, static_cast<GLsizei>( vertexCount ) );

        glBindVertexArray( 0 );

        vertices.clear();
    }

} // namespace sloth
