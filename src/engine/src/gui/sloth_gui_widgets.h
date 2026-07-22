#pragma once

#include "core/sloth_defines.h"
#include "core/sloth_string.h"
#include "gui/sloth_gui_frame.h"

#include <glm/glm.hpp>

namespace sloth {

    bool            Button( GuiFrame & frame, StringView label, glm::vec2 min, glm::vec2 max );
    bool            Checkbox( GuiFrame & frame, StringView label, glm::vec2 pos, bool & value );
    void            Label( GuiFrame & frame, StringView text, glm::vec2 baselinePos, f32 pixelHeight, const glm::vec4 & color );

    struct PanelResult {
        glm::vec2 contentMin;
        glm::vec2 contentMax;
    };

    PanelResult     BeginPanel( GuiFrame & frame, StringView label, glm::vec2 defaultPos, glm::vec2 size, bool draggable = true );
    void            EndPanel( GuiFrame & frame );

} // namespace sloth
