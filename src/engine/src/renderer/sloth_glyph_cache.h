#pragma once

#include "core/sloth_defines.h"

#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

namespace sloth
{
    class Font;

    // Per-glyph curve range/metrics into GlyphCache's curve buffer, plus the
    // metrics needed to place and size the glyph quad. All in font units.
    struct CachedGlyph
    {
        u32 CurveOffset = 0;
        u32 CurveCount = 0;
        i32 AdvanceWidth = 0;
        i32 XMin = 0;
        i32 YMin = 0;
        i32 XMax = 0;
        i32 YMax = 0;
    };

    // Lazily extracts glyph outlines from a Font and uploads them to a GPU
    // buffer texture (GL_TEXTURE_BUFFER, GL_RG32F) as a flat array of curve
    // control points, so each unique glyph is parsed and uploaded once no
    // matter how many times it's drawn. Curves for a glyph occupy a
    // contiguous range [CurveOffset, CurveOffset + CurveCount) of 3 texels
    // each (P0, P1, P2).
    class GlyphCache
    {
    public:
        GlyphCache();
        ~GlyphCache();

        SL_NON_COPYABLE(GlyphCache);
        SL_NON_MOVABLE(GlyphCache);

        // Extracts and uploads the glyph's outline on first request; returns
        // the cached entry on subsequent requests for the same glyph index.
        const CachedGlyph& GetOrAddGlyph(const Font& font, i32 glyphIndex);

        u32 GetCurveTextureId() const { return curveTexture; }

    private:
        // Uploads curveTexelsCpu[previousTexelCount:] to the GPU buffer,
        // growing (and re-uploading everything) if it doesn't fit.
        void UploadNewCurves(usize previousTexelCount);

    private:
        u32 curveBuffer = 0;  // GL buffer object backing the texture buffer.
        u32 curveTexture = 0; // GL_TEXTURE_BUFFER view over curveBuffer.
        usize bufferCapacityTexels = 0;

        std::vector<glm::vec2> curveTexelsCpu; // Mirrors the GPU buffer contents.
        std::unordered_map<i32, CachedGlyph> glyphs;
    };

} // namespace sloth
