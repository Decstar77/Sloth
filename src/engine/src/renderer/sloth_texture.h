#pragma once

#include "core/sloth_defines.h"

namespace sloth
{
    enum class TextureFilter : u8
    {
        Nearest, // No interpolation - crisp pixel-art icons.
        Linear,  // Bilinear interpolation - smooth scaling, the usual default for UI art.
    };

    enum class TextureWrap : u8
    {
        Clamp,  // Clamp to edge - the right default for UI images/icons (no edge bleeding).
        Repeat, // Tile - for background/pattern textures.
    };

    // A single GPU 2D texture. Always stored as RGBA8 regardless of the
    // source data's channel count, so sampling code never needs to branch on
    // format. Loading goes through stb_image (already compiled into the
    // engine via core/sloth_stb_impl.cpp); on failure IsLoaded() is false
    // and the texture is safe to destroy but not to Bind().
    class Texture
    {
    public:
        // Loads an image file from disk (PNG/JPG/BMP/... - whatever
        // stb_image supports).
        explicit Texture(const char* path, TextureFilter filter = TextureFilter::Linear, TextureWrap wrap = TextureWrap::Clamp);

        // Builds a texture directly from an in-memory RGBA8 pixel buffer
        // (width * height * 4 bytes, row-major, top row first).
        Texture(const u8* pixelsRgba, i32 width, i32 height, TextureFilter filter = TextureFilter::Linear, TextureWrap wrap = TextureWrap::Clamp);
        ~Texture();

        SL_NON_COPYABLE(Texture);
        SL_NON_MOVABLE(Texture);

        void Bind(u32 slot = 0) const;

        bool IsLoaded() const { return rendererId != 0; }
        i32 GetWidth() const { return width; }
        i32 GetHeight() const { return height; }
        u32 GetRendererId() const { return rendererId; }

    private:
        void CreateGLTexture(const u8* pixelsRgba, TextureFilter filter, TextureWrap wrap);

    private:
        u32 rendererId = 0;
        i32 width = 0;
        i32 height = 0;
    };

} // namespace sloth
