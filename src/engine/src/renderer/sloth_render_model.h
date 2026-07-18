#pragma once

namespace sloth
{
    class Shader;
    class StaticMesh;

    struct RenderModel
    {
        Shader*     shader = nullptr;
        StaticMesh* mesh   = nullptr;
    };

} // namespace sloth
