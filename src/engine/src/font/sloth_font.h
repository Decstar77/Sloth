#pragma once

#include "core/sloth_defines.h"

#include <glm/glm.hpp>
#include <vector>

struct stbtt_fontinfo;

namespace sloth {
    class Arena;

    // Ascent/descent/line gap in font units - multiply by
    // Font::GetScaleForPixelHeight() to convert to pixels.
    struct FontVMetrics {
        i32 ascent = 0;
        i32 descent = 0;
        i32 lineGap = 0;
    };

    // Advance width / left side bearing in font units, for a single glyph.
    struct GlyphMetrics {
        i32 advanceWidth = 0;
        i32 leftSideBearing = 0;
    };

    // A single quadratic Bezier contour segment, in font units. Straight
    // line segments (as produced by stb_truetype for on-curve-to-on-curve
    // spans) are represented as a degenerate curve with P1 = midpoint(P0,
    // P2), so consumers only ever need to handle one segment type.
    struct GlyphCurve {
        glm::vec2 p0;
        glm::vec2 p1;
        glm::vec2 p2;
    };

    // A glyph's outline as a flat list of curves (contours are implicit -
    // each contour is a closed loop of consecutive curves, but callers that
    // only care about filling the shape don't need contour boundaries) plus
    // its metrics/bounding box, all in font units.
    struct GlyphOutline {
        std::vector<GlyphCurve> curves;
        i32 advanceWidth = 0;
        i32 leftSideBearing = 0;
        i32 xMin = 0;
        i32 yMin = 0;
        i32 xMax = 0;
        i32 yMax = 0;
    };

    // Loads a TrueType/OpenType font file (.ttf/.otf/.ttc) and exposes glyph
    // lookup, metrics, and outline extraction (as quadratic Bezier contours,
    // for the Slug renderer) on top of stb_truetype. Parsing only - no
    // rasterization.
    //
    // The raw font file bytes and the parsed stbtt_fontinfo are both pushed
    // from `arena` and live as long as the arena does, so Font itself owns
    // no heap memory that needs freeing.
    class Font {
      public:
        explicit Font( Arena* arena );

        SL_NON_COPYABLE( Font );
        SL_NON_MOVABLE( Font );

        // Reads the file at `path` and parses it. Returns false (and logs)
        // on I/O failure or if stb_truetype doesn't recognize the format.
        bool Load( const char* path );
        bool IsLoaded() const {
            return loaded;
        }

        // Unicode codepoint -> glyph index. Returns 0 (the "missing glyph")
        // if the font has no mapping for it.
        i32 GetGlyphIndex( u32 codepoint ) const;
        i32 GetGlyphCount() const;

        // Font-unit-to-pixel scale factor for a desired pixel height.
        f32 GetScaleForPixelHeight( f32 pixelHeight ) const;

        FontVMetrics GetVMetrics() const;
        GlyphMetrics GetGlyphMetrics( i32 glyphIndex ) const;

        // Extracts the glyph's outline as a flat list of quadratic Bezier
        // curves (see GlyphCurve). Returns false (and logs) if the glyph
        // uses cubic curves, which TrueType glyf outlines never do - only
        // relevant for CFF/OpenType-CFF fonts, unsupported for now.
        bool GetGlyphOutline( i32 glyphIndex, GlyphOutline& outOutline ) const;

      private:
        Arena* arena = nullptr;
        stbtt_fontinfo* fontInfo = nullptr;
        const u8* fileData = nullptr;
        bool loaded = false;
    };

} // namespace sloth
