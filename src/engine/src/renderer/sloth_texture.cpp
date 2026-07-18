#include "sloth_texture.h"

#include <glad/gl.h>
#include <stb_image.h>

namespace sloth {
    static GLenum ToGLFilter( TextureFilter filter ) {
        return filter == TextureFilter::Nearest ? GL_NEAREST : GL_LINEAR;
    }

    static GLenum ToGLWrap( TextureWrap wrap ) {
        return wrap == TextureWrap::Repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    }

    Texture::Texture( const char * path, TextureFilter filter, TextureWrap wrap ) {
        i32 sourceChannels = 0;
        u8 * pixels = stbi_load( path, &width, &height, &sourceChannels, STBI_rgb_alpha );
        if ( !pixels ) {
            SL_LOG_ERROR( "Failed to load texture '%s': %s", path, stbi_failure_reason() );
            width = 0;
            height = 0;
            return;
        }

        CreateGLTexture( pixels, filter, wrap );
        stbi_image_free( pixels );
    }

    Texture::Texture( const u8 * pixelsRgba, i32 textureWidth, i32 textureHeight, TextureFilter filter, TextureWrap wrap )
        : width( textureWidth ), height( textureHeight ) {
        CreateGLTexture( pixelsRgba, filter, wrap );
    }

    Texture::~Texture() {
        glDeleteTextures( 1, &rendererId );
    }

    void Texture::CreateGLTexture( const u8 * pixelsRgba, TextureFilter filter, TextureWrap wrap ) {
        glGenTextures( 1, &rendererId );
        glBindTexture( GL_TEXTURE_2D, rendererId );

        GLenum glWrap = ToGLWrap( wrap );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>( glWrap ) );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>( glWrap ) );

        GLenum glFilter = ToGLFilter( filter );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>( glFilter ) );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>( glFilter ) );

        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRgba );

        glBindTexture( GL_TEXTURE_2D, 0 );
    }

    void Texture::Bind( u32 slot ) const {
        glActiveTexture( GL_TEXTURE0 + slot );
        glBindTexture( GL_TEXTURE_2D, rendererId );
    }

} // namespace sloth
