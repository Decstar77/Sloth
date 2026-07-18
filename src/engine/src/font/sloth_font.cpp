#include "sloth_font.h"

#include "core/sloth_arena.h"

#include <stb_truetype.h>

#include <cstdio>

namespace sloth
{
    Font::Font(Arena* arena) : arena(arena)
    {
        SL_ASSERT_MSG(arena != nullptr, "Font requires a non-null arena");
    }

    bool Font::Load(const char* path)
    {
        loaded = false;
        fontInfo = nullptr;
        fileData = nullptr;

        std::FILE* file = std::fopen(path, "rb");
        if (file == nullptr)
        {
            SL_LOG_ERROR("Font: failed to open '%s'", path);
            return false;
        }

        std::fseek(file, 0, SEEK_END);
        long fileSize = std::ftell(file);
        if (fileSize <= 0)
        {
            SL_LOG_ERROR("Font: '%s' is empty or unreadable", path);
            std::fclose(file);
            return false;
        }
        std::fseek(file, 0, SEEK_SET);

        u8* data = arena->PushArray<u8>(static_cast<usize>(fileSize));
        usize readBytes = std::fread(data, 1, static_cast<usize>(fileSize), file);
        std::fclose(file);

        if (readBytes != static_cast<usize>(fileSize))
        {
            SL_LOG_ERROR("Font: failed to read '%s' (read %zu of %ld bytes)", path, readBytes, fileSize);
            return false;
        }

        stbtt_fontinfo* info = arena->PushStruct<stbtt_fontinfo>();
        i32 offset = stbtt_GetFontOffsetForIndex(data, 0);
        if (offset < 0 || !stbtt_InitFont(info, data, offset))
        {
            SL_LOG_ERROR("Font: '%s' is not a recognizable TrueType/OpenType font", path);
            return false;
        }

        fileData = data;
        fontInfo = info;
        loaded = true;

        SL_LOG_INFO("Font: loaded '%s' (%d glyphs)", path, GetGlyphCount());
        return true;
    }

    i32 Font::GetGlyphIndex(u32 codepoint) const
    {
        SL_ASSERT_MSG(loaded, "Font::GetGlyphIndex called before a successful Load()");
        return stbtt_FindGlyphIndex(fontInfo, static_cast<i32>(codepoint));
    }

    i32 Font::GetGlyphCount() const
    {
        SL_ASSERT_MSG(loaded, "Font::GetGlyphCount called before a successful Load()");
        return fontInfo->numGlyphs;
    }

    f32 Font::GetScaleForPixelHeight(f32 pixelHeight) const
    {
        SL_ASSERT_MSG(loaded, "Font::GetScaleForPixelHeight called before a successful Load()");
        return stbtt_ScaleForPixelHeight(fontInfo, pixelHeight);
    }

    FontVMetrics Font::GetVMetrics() const
    {
        SL_ASSERT_MSG(loaded, "Font::GetVMetrics called before a successful Load()");
        FontVMetrics metrics;
        stbtt_GetFontVMetrics(fontInfo, &metrics.Ascent, &metrics.Descent, &metrics.LineGap);
        return metrics;
    }

    GlyphMetrics Font::GetGlyphMetrics(i32 glyphIndex) const
    {
        SL_ASSERT_MSG(loaded, "Font::GetGlyphMetrics called before a successful Load()");
        GlyphMetrics metrics;
        stbtt_GetGlyphHMetrics(fontInfo, glyphIndex, &metrics.AdvanceWidth, &metrics.LeftSideBearing);
        return metrics;
    }

    bool Font::GetGlyphOutline(i32 glyphIndex, GlyphOutline& outOutline) const
    {
        SL_ASSERT_MSG(loaded, "Font::GetGlyphOutline called before a successful Load()");

        outOutline.Curves.clear();

        GlyphMetrics hMetrics = GetGlyphMetrics(glyphIndex);
        outOutline.AdvanceWidth = hMetrics.AdvanceWidth;
        outOutline.LeftSideBearing = hMetrics.LeftSideBearing;

        stbtt_GetGlyphBox(fontInfo, glyphIndex, &outOutline.XMin, &outOutline.YMin, &outOutline.XMax, &outOutline.YMax);

        stbtt_vertex* vertices = nullptr;
        i32 vertexCount = stbtt_GetGlyphShape(fontInfo, glyphIndex, &vertices);
        if (vertexCount <= 0)
        {
            // Empty glyph (e.g. space) - not an error.
            return true;
        }

        bool ok = true;
        glm::vec2 current(0.0f, 0.0f);

        for (i32 i = 0; i < vertexCount; ++i)
        {
            const stbtt_vertex& v = vertices[i];
            glm::vec2 p(static_cast<f32>(v.x), static_cast<f32>(v.y));

            switch (v.type)
            {
                case STBTT_vmove:
                    current = p;
                    break;

                case STBTT_vline:
                {
                    glm::vec2 mid = 0.5f * (current + p);
                    outOutline.Curves.push_back({ current, mid, p });
                    current = p;
                    break;
                }

                case STBTT_vcurve:
                {
                    glm::vec2 control(static_cast<f32>(v.cx), static_cast<f32>(v.cy));
                    outOutline.Curves.push_back({ current, control, p });
                    current = p;
                    break;
                }

                case STBTT_vcubic:
                    SL_LOG_WARN("Font: glyph %d has a cubic curve segment, which is unsupported - outline will be incomplete", glyphIndex);
                    ok = false;
                    current = p;
                    break;

                default:
                    break;
            }
        }

        stbtt_FreeShape(fontInfo, vertices);
        return ok;
    }

} // namespace sloth
