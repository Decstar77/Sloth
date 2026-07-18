#include "sloth_glyph_cache.h"

#include "font/sloth_font.h"

#include <glad/gl.h>

namespace sloth
{
    GlyphCache::GlyphCache()
    {
        glGenBuffers(1, &curveBuffer);
        glGenTextures(1, &curveTexture);
    }

    GlyphCache::~GlyphCache()
    {
        glDeleteTextures(1, &curveTexture);
        glDeleteBuffers(1, &curveBuffer);
    }

    const CachedGlyph& GlyphCache::GetOrAddGlyph(const Font& font, i32 glyphIndex)
    {
        auto it = glyphs.find(glyphIndex);
        if (it != glyphs.end())
        {
            return it->second;
        }

        GlyphOutline outline;
        font.GetGlyphOutline(glyphIndex, outline);

        CachedGlyph cached;
        cached.CurveOffset = static_cast<u32>(curveTexelsCpu.size() / 3);
        cached.CurveCount = static_cast<u32>(outline.Curves.size());
        cached.AdvanceWidth = outline.AdvanceWidth;
        cached.XMin = outline.XMin;
        cached.YMin = outline.YMin;
        cached.XMax = outline.XMax;
        cached.YMax = outline.YMax;

        usize previousTexelCount = curveTexelsCpu.size();
        for (const GlyphCurve& curve : outline.Curves)
        {
            curveTexelsCpu.push_back(curve.P0);
            curveTexelsCpu.push_back(curve.P1);
            curveTexelsCpu.push_back(curve.P2);
        }

        if (curveTexelsCpu.size() > previousTexelCount)
        {
            UploadNewCurves(previousTexelCount);
        }

        return glyphs.emplace(glyphIndex, cached).first->second;
    }

    void GlyphCache::UploadNewCurves(usize previousTexelCount)
    {
        usize requiredCapacity = curveTexelsCpu.size();
        glBindBuffer(GL_TEXTURE_BUFFER, curveBuffer);

        if (requiredCapacity > bufferCapacityTexels)
        {
            usize newCapacity = bufferCapacityTexels == 0 ? 256 : bufferCapacityTexels;
            while (newCapacity < requiredCapacity)
            {
                newCapacity *= 2;
            }

            glBufferData(GL_TEXTURE_BUFFER, static_cast<GLsizeiptr>(newCapacity * sizeof(glm::vec2)), nullptr, GL_DYNAMIC_DRAW);
            glBufferSubData(GL_TEXTURE_BUFFER, 0, static_cast<GLsizeiptr>(curveTexelsCpu.size() * sizeof(glm::vec2)), curveTexelsCpu.data());
            bufferCapacityTexels = newCapacity;

            glBindTexture(GL_TEXTURE_BUFFER, curveTexture);
            glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, curveBuffer);
        }
        else
        {
            usize newCount = requiredCapacity - previousTexelCount;
            glBufferSubData(GL_TEXTURE_BUFFER, static_cast<GLintptr>(previousTexelCount * sizeof(glm::vec2)),
                             static_cast<GLsizeiptr>(newCount * sizeof(glm::vec2)), curveTexelsCpu.data() + previousTexelCount);
        }
    }

} // namespace sloth
