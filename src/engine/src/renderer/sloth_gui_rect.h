#pragma once

#include "core/sloth_defines.h"

#include <glm/glm.hpp>

namespace sloth
{
    // An axis-aligned screen-space rectangle in pixels (y-down), used for
    // clip regions. Both GuiRenderer (which clips per-fragment in its
    // shaders) and GuiContext (which uses it to gate hit-testing) keep their
    // own clip-rect stack built from this same type and intersection rule -
    // one is a rendering concern, the other an interaction concern, and
    // they're deliberately not merged into one shared stack (see the
    // GuiContext/GuiRenderer class comments).
    struct GuiRect
    {
        glm::vec2 min;
        glm::vec2 max;

        bool Contains(glm::vec2 point) const
        {
            return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
        }
    };

    // No bound at all - the base of an empty clip stack (nothing clipped
    // yet), and the identity element for IntersectGuiRect().
    inline GuiRect UnboundedGuiRect()
    {
        constexpr f32 kLarge = 1e30f;
        return GuiRect{ glm::vec2(-kLarge), glm::vec2(kLarge) };
    }

    inline GuiRect IntersectGuiRect(const GuiRect& a, const GuiRect& b)
    {
        return GuiRect{ glm::max(a.min, b.min), glm::min(a.max, b.max) };
    }

} // namespace sloth
